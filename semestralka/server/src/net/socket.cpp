#include "net/error.h"
#include "utils/functional.h"
#include <fcntl.h>
#include <limits>
#include <net/address.h>
#include <net/socket.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace net {

error::result<Socket> Socket::create(int family, int type) {
  int fd = socket(family, type | SOCK_CLOEXEC, 0);

  return error::from_os(fd).map(functional::BindConstructor<Socket>());
}

error::result<void> Socket::bind_to(const Address &addr) const {
  const auto [sockaddr_union, len] = addr.to_sockaddr();

  const auto *const sockaddr =
      reinterpret_cast<const struct sockaddr *const>(&sockaddr_union);

  return error::from_os(bind(raw_fd(), sockaddr, len)).map(functional::drop);
}

error::result<Socket>
Socket::accept(sockaddr &storage, socklen_t &len, int flags) const {
  return error::from_os(accept4(raw_fd(), &storage, &len, SOCK_CLOEXEC | flags))
      .map(functional::BindConstructor<Socket>());
}

error::result<void> Socket::connect(const Address &addr) const {
  const auto [sockaddr_union, len] = addr.to_sockaddr();

  while (true) {
    const auto result = ::connect(
        raw_fd(),
        reinterpret_cast<const struct sockaddr *const>(&sockaddr_union),
        len
    );

    if (result != -1 || errno == EISCONN) {
      return {};
    }

    if (errno == EINTR) {
      continue;
    }

    return error::from_os(result).map(functional::drop);
  }
}

error::result<void> Socket::connect_timeout(
    const Address &addr, std::chrono::microseconds timeout
) const {
  const auto [sockaddr_union, len] = addr.to_sockaddr();

  if (const auto error = set_nonblocking(true); !error) {
    return error;
  }
  const auto result = error::from_os(::connect(
      raw_fd(),
      reinterpret_cast<const struct sockaddr *const>(&sockaddr_union),
      len
  ));
  if (const auto error = set_nonblocking(false); !error) {
    return error;
  }

  if (result.has_value()) {
    // Connection succeeded immediately
    return {};
  }

  if (result.error().os_code().value_or(0) != EINPROGRESS) {
    // Connection failed immediately
    return result.map(functional::drop);
  }

  struct pollfd pfd{.fd = raw_fd(), .events = POLLOUT, .revents = 0};

  if (timeout.count() == 0) {
    // No timeout
    return tl::make_unexpected(error::SimpleMessage(
        error::ErrorKind::InvalidInput, "Timeout can't be zero"
    ));
  }

  const auto start_time = std::chrono::steady_clock::now();

  while (true) {
    const auto elapsed = std::chrono::steady_clock::now() - start_time;
    if (elapsed >= timeout) {
      return tl::make_unexpected(error::SimpleMessage(
          error::ErrorKind::TimedOut, "Connection timed out"
      ));
    }

    const auto remaining_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout - elapsed)
            .count();
    int poll_timeout = static_cast<int>(std::min(
        remaining_time, static_cast<long>(std::numeric_limits<int>::max())
    ));

    if (poll_timeout == 0) {
      poll_timeout = 1;
    }

    const int poll_result = poll(&pfd, 1, poll_timeout);

    if (poll_result == -1) {
      if (errno == EINTR) {
        continue;
      }
      return tl::make_unexpected(error::last_os_error());
    }

    if (poll_result > 0) {
      if ((pfd.revents & (POLLHUP | POLLERR)) != 0) {
        const auto error_result = error_state();
        if (!error_result) {
          return tl::make_unexpected(error_result.error());
        }

        const auto &error = error_result.value();
        if (error) {
          return tl::make_unexpected(error.value());
        }

        return tl::make_unexpected(error::SimpleMessage(
            error::ErrorKind::Uncategorized, "No error set after POLLHUP"
        ));
      }

      // Connection succeeded
      return {};
    }
  }
}

error::result<std::optional<error::IoError>> Socket::error_state() const {
  const auto result = getopts<int>(SOL_SOCKET, SO_ERROR);

  if (!result) {
    return tl::make_unexpected(result.error());
  }

  if (*result == 0) {
    return std::nullopt;
  }

  return error::Os{*result};
}

error::result<void> Socket::set_nonblocking(bool nonblocking) const {
  // There is no other way to do this
  // NOLINTNEXTLINE(*cppcoreguidelines-pro-type-vararg)
  const auto code = ::fcntl(raw_fd(), F_SETFL, nonblocking ? O_NONBLOCK : 0);
  return error::from_os(code).map(functional::drop);
}

ssize_t Socket::read(void *buf, const size_t len) const {
  return ::read(raw_fd(), buf, len);
}

ssize_t Socket::write(const void *buf, const size_t len) const {
  return ::write(raw_fd(), buf, len);
}

ssize_t Socket::recv(void *buf, const size_t len, int flags) const {
  return ::recv(raw_fd(), buf, len, flags);
}

ssize_t Socket::send(const void *buf, const size_t len, int flags) const {
  return ::send(raw_fd(), buf, len, flags);
}

} // namespace net
