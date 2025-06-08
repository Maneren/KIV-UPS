#include <fcntl.h>
#include <limits>
#include <net/address.h>
#include <net/exception.h>
#include <net/socket.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace net {

Socket::Socket(int family, int type) {
  int fd = socket(family, type | SOCK_CLOEXEC, 0);

  if (fd < 0) {
    throw io_exception("failed to create socket");
  }

  this->fd = fd;
}

void Socket::bind_to(const Address &addr) const {
  const auto [sockaddr_union, len] = addr.to_sockaddr();

  const auto sockaddr =
      reinterpret_cast<const struct sockaddr *>(&sockaddr_union);

  if (bind(fd.fd, sockaddr, len) < 0) {
    throw io_exception("failed to bind socket");
  }
}

Socket Socket::accept(sockaddr &storage, socklen_t &len, int flags) const {
  const auto fd = accept4(this->fd.fd, &storage, &len, SOCK_CLOEXEC | flags);

  if (fd < 0) {
    throw io_exception("failed to accept connection");
  }

  return {FileDescriptor{fd}};
}

void Socket::connect(const Address &addr) const {
  const auto [sockaddr_union, len] = addr.to_sockaddr();

  while (true) {
    const auto result = ::connect(
        fd.fd, reinterpret_cast<const struct sockaddr *>(&sockaddr_union), len
    );

    if (result != -1 || errno == EISCONN) {
      return;
    }

    if (errno == EINTR) {
      continue;
    }

    throw io_exception("failed to connect socket", errno);
  }
}

void Socket::connect_timeout(
    const Address &addr, std::chrono::microseconds timeout
) const {
  const auto [sockaddr_union, len] = addr.to_sockaddr();

  set_nonblocking(true);
  const auto result = ::connect(
      fd.fd, reinterpret_cast<const struct sockaddr *>(&sockaddr_union), len
  );
  set_nonblocking(false);

  if (result == 0) {
    // Connection succeeded immediately
    return;
  }

  if (errno != EINPROGRESS) {
    // Connection failed immediately
    throw io_exception("failed to connect socket", errno);
  }

  struct pollfd pfd{fd.fd, POLLOUT, 0};

  if (timeout.count() == 0) {
    // No timeout
    throw io_exception("timeout can't be zero");
  }

  const auto start_time = std::chrono::steady_clock::now();

  while (true) {
    const auto elapsed = std::chrono::steady_clock::now() - start_time;
    if (elapsed >= timeout) {
      throw io_exception("connection timed out");
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
      throw io_exception("poll failed", errno);
    }

    if (poll_result > 0) {
      if ((pfd.revents & (POLLHUP | POLLERR)) != 0) {
        const auto error = error_state();
        if (error != 0) {
          throw io_exception("connection failed", error);
        }
        throw io_exception("connection failed", errno);
      }

      // Connection succeeded
      return;
    }
  }
}

int Socket::error_state() const { return getopts<int>(SOL_SOCKET, SO_ERROR); }

void Socket::set_nonblocking(bool nonblocking) const {
  if (::fcntl(fd.fd, F_SETFL, nonblocking ? O_NONBLOCK : 0) < 0) {
    throw io_exception("failed to set socket non-blocking");
  }
}

ssize_t Socket::read(void *buf, const size_t len) const {
  return ::read(fd.fd, buf, len);
}

ssize_t Socket::write(const void *buf, const size_t len) const {
  return ::write(fd.fd, buf, len);
}

ssize_t Socket::recv(void *buf, const size_t len, int flags) const {
  return ::recv(fd.fd, buf, len, flags);
}

ssize_t Socket::send(const void *buf, const size_t len, int flags) const {
  return ::send(fd.fd, buf, len, flags);
}

} // namespace net
