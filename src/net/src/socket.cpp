#include <fcntl.h>
#include <limits>
#include <net/address.h>
#include <net/error.h>
#include <net/socket.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <utils/functional.h>

namespace net {

error::result<Socket> Socket::create(int family, int type) {
  int fd = socket(family, type | SOCK_CLOEXEC, 0);

  return error::from_os(fd).map([](int raw) {
    return Socket(FileDescriptor(raw));
  });
}

error::result<void> Socket::bind_to(const Address &addr) const {
  const auto [sockaddr_union, len] = addr.to_sockaddr();

  const auto *const sockaddr =
      reinterpret_cast<const struct sockaddr *const>(&sockaddr_union);

  return error::from_os(bind(raw_fd(), sockaddr, len)).map(functional::drop);
}

error::result<Socket>
Socket::accept(sockaddr &storage, socklen_t &len, int flags) const {
  while (true) {
    const int raw = accept4(raw_fd(), &storage, &len, SOCK_CLOEXEC | flags);
    if (raw != -1) {
      return Socket(FileDescriptor(raw));
    }
    if (errno == EINTR) {
      continue;
    }
    return tl::make_unexpected(error::Os{errno});
  }
}

error::result<void> Socket::connect(const Address &addr) const {
  const auto [sockaddr_union, len] = addr.to_sockaddr();

  const auto *const sockaddr =
      reinterpret_cast<const struct sockaddr *const>(&sockaddr_union);

  while (true) {
    const auto result = ::connect(raw_fd(), sockaddr, len);

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
  if (timeout.count() <= 0) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Timeout must be positive"
        )
    );
  }

  const auto [sockaddr_union, len] = addr.to_sockaddr();

  if (const auto made = set_nonblocking(true); !made) {
    return tl::make_unexpected(made.error());
  }

  const auto restore_blocking = [this]() -> error::result<void> {
    return set_nonblocking(false);
  };

  const auto *const sockaddr =
      reinterpret_cast<const struct sockaddr *const>(&sockaddr_union);

  const int connect_result = ::connect(raw_fd(), sockaddr, len);

  if (connect_result == 0) {
    return restore_blocking();
  }

  const int connect_errno = errno;
  if (connect_errno != EINPROGRESS && connect_errno != EINTR) {
    const auto restore = restore_blocking();
    if (!restore) {
      return tl::make_unexpected(restore.error());
    }
    return tl::make_unexpected(error::Os{connect_errno});
  }

  struct pollfd pfd{.fd = raw_fd(), .events = POLLOUT, .revents = 0};

  const auto start_time = std::chrono::steady_clock::now();

  while (true) {
    const auto elapsed = std::chrono::steady_clock::now() - start_time;
    if (elapsed >= timeout) {
      const auto restore = restore_blocking();
      if (!restore) {
        return tl::make_unexpected(restore.error());
      }
      return tl::make_unexpected(
          error::SimpleMessage(
              error::ErrorKind::TimedOut, "Connection timed out"
          )
      );
    }

    const auto remaining = timeout - elapsed;
    const auto remaining_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(remaining);
    // Round up so sub-millisecond timeouts still wait.
    long millis = static_cast<long>(remaining_ms.count());
    if (remaining > remaining_ms) {
      ++millis;
    }
    millis = std::min<long>(
        millis, static_cast<long>(std::numeric_limits<int>::max())
    );
    const int poll_timeout = static_cast<int>(std::max<long>(millis, 1));

    const int poll_result = poll(&pfd, 1, poll_timeout);

    if (poll_result == -1) {
      if (errno == EINTR) {
        continue;
      }
      const int poll_errno = errno;
      const auto restore = restore_blocking();
      if (!restore) {
        return tl::make_unexpected(restore.error());
      }
      return tl::make_unexpected(error::Os{poll_errno});
    }

    if (poll_result > 0) {
      // A writable socket may still carry a pending async error, so
      // always consult SO_ERROR (not only on POLLERR/POLLHUP).
      const auto error_result = error_state();
      if (!error_result) {
        const auto restore = restore_blocking();
        if (!restore) {
          return tl::make_unexpected(restore.error());
        }
        return tl::make_unexpected(error_result.error());
      }

      const auto &pending = error_result.value();
      if (pending) {
        const auto restore = restore_blocking();
        if (!restore) {
          return tl::make_unexpected(restore.error());
        }
        return tl::make_unexpected(pending.value());
      }

      return restore_blocking();
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
  const int current = ::fcntl(raw_fd(), F_GETFL, 0);
  if (current == -1) {
    return tl::make_unexpected(error::Os{errno});
  }
  const int updated =
      nonblocking ? (current | O_NONBLOCK) : (current & ~O_NONBLOCK);
  if (updated == current) {
    return {};
  }
  // NOLINTNEXTLINE(*cppcoreguidelines-pro-type-vararg)
  return error::from_os(::fcntl(raw_fd(), F_SETFL, updated))
      .map(functional::drop);
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
