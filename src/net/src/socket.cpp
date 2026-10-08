#include <cerrno>
#include <fcntl.h>
#include <limits>
#include <net/error.h>
#include <net/socket.h>
#include <net/socket_addr.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <utils/functional.h>

namespace net {

namespace {

template <typename F>
auto retry_on_eintr(F &&fn) -> error::result<std::invoke_result_t<F>> {
  while (true) {
    if (auto ret = std::forward<F>(fn)(); ret >= 0) {
      return ret;
    }
    if (errno == EINTR) {
      continue;
    }
    return tl::make_unexpected(error::Os{});
  }
}

} // namespace

Socket::Socket(FileDescriptor &&fd) : fd(std::move(fd)) {}

Socket::Socket(Socket &&other) noexcept : fd(std::move(other.fd)) {}

Socket &Socket::operator=(Socket &&other) noexcept {
  fd = std::move(other.fd);
  return *this;
}

error::result<Socket> Socket::create(int family, int type) {
  const int fd = socket(family, type | SOCK_CLOEXEC, 0);

  return error::from_os(fd).map([](int raw) {
    return Socket(FileDescriptor(raw));
  });
}

error::result<Socket> Socket::create(const SocketAddr &addr, int type) {
  return create(addr.family(), type);
}

error::result<Socket> Socket::duplicate() const {
  return fd.duplicate().map(functional::Constructor<Socket>());
}

error::result<void> Socket::bind_to(const SocketAddr &addr) const {
  const auto [sockaddr, len] = addr.to_sockaddr();

  return error::from_os(bind(raw_fd(), &sockaddr.sa, len))
      .map(functional::drop);
}

error::result<Socket>
Socket::accept(sockaddr_union &sockaddr, socklen_t &len, int flags) const {
  return retry_on_eintr([&] {
           return accept4(raw_fd(), &sockaddr.sa, &len, flags | SOCK_CLOEXEC);
         })
      .map([](int raw) { return Socket(FileDescriptor(raw)); });
}

error::result<void> Socket::connect(const SocketAddr &addr) const {
  const auto [storage, len] = addr.to_sockaddr();

  while (true) {
    const auto result = ::connect(raw_fd(), &storage.sa, len);

    if (result != -1 || errno == EISCONN) {
      return {};
    }

    if (errno == EINTR) {
      continue;
    }

    return tl::make_unexpected(error::Os{errno});
  }
}

error::result<void> Socket::connect_timeout(
    const SocketAddr &addr, std::chrono::microseconds timeout
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

  const auto restore_blocking = [this] -> error::result<void> {
    return set_nonblocking(false);
  };

  const int connect_result = ::connect(raw_fd(), &sockaddr_union.sa, len);

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
      const auto error_result = take_error();
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

error::result<std::optional<error::IoError>> Socket::take_error() const {
  const auto result = getopt<int>(SOL_SOCKET, SO_ERROR);

  if (!result) {
    return tl::make_unexpected(result.error());
  }

  if (*result == 0) {
    return std::nullopt;
  }

  return error::Os{*result};
}

namespace {

int enable_bitflag(int base, int flag) {
  return static_cast<int>(
      static_cast<unsigned>(base) | static_cast<unsigned>(flag)
  );
}
int disable_bitflag(int base, int flag) {
  return static_cast<int>(
      static_cast<unsigned>(base) & ~static_cast<unsigned>(flag)
  );
}

} // namespace

error::result<void> Socket::set_nonblocking(bool nonblocking) const {
  // There is no other way to do this
  // NOLINTNEXTLINE(*cppcoreguidelines-pro-type-vararg)
  const int current = ::fcntl(raw_fd(), F_GETFL, 0);
  if (current == -1) {
    return tl::make_unexpected(error::Os{errno});
  }
  const int updated = nonblocking ? enable_bitflag(current, O_NONBLOCK)
                                  : disable_bitflag(current, O_NONBLOCK);
  if (updated == current) {
    return {};
  }
  // NOLINTNEXTLINE(*cppcoreguidelines-pro-type-vararg)
  return error::from_os(::fcntl(raw_fd(), F_SETFL, updated))
      .map(functional::drop);
}

error::result<ssize_t> Socket::read(void *buf, const size_t len) const {
  return retry_on_eintr([&] { return ::read(raw_fd(), buf, len); });
}

error::result<ssize_t> Socket::write(const void *buf, const size_t len) const {
  return retry_on_eintr([&] { return ::write(raw_fd(), buf, len); });
}

error::result<ssize_t>
Socket::recv(void *buf, const size_t len, int flags) const {
  return retry_on_eintr([&] { return ::recv(raw_fd(), buf, len, flags); });
}

error::result<ssize_t>
Socket::send(const void *buf, const size_t len, int flags) const {
  return retry_on_eintr([&] { return ::send(raw_fd(), buf, len, flags); });
}

error::result<SocketAddr> Socket::local_addr() const {
  sockaddr_union sockaddr{};
  auto len = sockaddr_union::SIZE;
  if (getsockname(raw_fd(), &sockaddr.sa, &len) == -1) {
    return tl::make_unexpected(error::Os{errno});
  }
  return SocketAddr::from_sockaddr(sockaddr, len);
}

error::result<SocketAddr> Socket::peer_addr() const {
  sockaddr_union sockaddr{};
  auto len = sockaddr_union::SIZE;
  if (getpeername(raw_fd(), &sockaddr.sa, &len) == -1) {
    return tl::make_unexpected(error::Os{errno});
  }
  return SocketAddr::from_sockaddr(sockaddr, len);
}

error::result<void> Socket::shutdown(Shutdown how) const {
  int flag = 0;
  switch (how) {
  case Shutdown::Read:
    flag = SHUT_RD;
    break;
  case Shutdown::Write:
    flag = SHUT_WR;
    break;
  case Shutdown::Both:
    flag = SHUT_RDWR;
    break;
  }
  return error::from_os(::shutdown(raw_fd(), flag)).map(functional::drop);
}

error::result<void> Socket::set_reuseaddr(bool reuse) const {
  const int opt = reuse ? 1 : 0;
  return setopt(SOL_SOCKET, SO_REUSEADDR, opt);
}

error::result<void> Socket::set_nodelay(bool nodelay) const {
  const int opt = nodelay ? 1 : 0;
  return setopt(IPPROTO_TCP, TCP_NODELAY, opt);
}

error::result<bool> Socket::nodelay() const {
  return getopt<int>(IPPROTO_TCP, TCP_NODELAY).map([](int opt) {
    return opt != 0;
  });
}

error::result<void> Socket::set_ttl(uint32_t ttl) const {
  const int opt = static_cast<int>(ttl);
  // Try IPv4 first, fall back to IPv6 unicast hops so unbound or
  // either-family sockets work without the caller caring.
  if (const auto v4 = setopt(IPPROTO_IP, IP_TTL, opt); v4) {
    return {};
  }
  const int opt6 = static_cast<int>(ttl);
  return setopt(IPPROTO_IPV6, IPV6_UNICAST_HOPS, opt6);
}

error::result<uint32_t> Socket::ttl() const {
  if (const auto addr = local_addr(); addr) {
    if (addr->family() == AF_INET6) {
      return getopt<int>(IPPROTO_IPV6, IPV6_UNICAST_HOPS).map([](int opt) {
        return static_cast<uint32_t>(opt);
      });
    }
    return getopt<int>(IPPROTO_IP, IP_TTL).map([](int opt) {
      return static_cast<uint32_t>(opt);
    });
  }
  // Unbound: try v4, then v6.
  if (auto v4 = getopt<int>(IPPROTO_IP, IP_TTL); v4) {
    return static_cast<uint32_t>(*v4);
  }
  return getopt<int>(IPPROTO_IPV6, IPV6_UNICAST_HOPS).map([](int opt) {
    return static_cast<uint32_t>(opt);
  });
}

namespace {

error::result<void> set_socket_timeout(
    const Socket &sock,
    int optname,
    std::optional<std::chrono::microseconds> timeout
) {
  struct timeval tv{};
  if (timeout) {
    if (timeout->count() < 0) {
      return tl::make_unexpected(
          error::SimpleMessage(
              error::ErrorKind::InvalidInput, "Timeout must be non-negative"
          )
      );
    }
    const auto secs =
        std::chrono::duration_cast<std::chrono::seconds>(*timeout);
    const auto usecs =
        std::chrono::duration_cast<std::chrono::microseconds>(*timeout - secs);
    tv.tv_sec = secs.count();
    tv.tv_usec = usecs.count();
  }
  return sock.setopt(SOL_SOCKET, optname, tv);
}

error::result<std::optional<std::chrono::microseconds>>
get_socket_timeout(const Socket &sock, int optname) {
  return sock.getopt<struct timeval>(SOL_SOCKET, optname)
      .map([](struct timeval tv) -> std::optional<std::chrono::microseconds> {
        if (tv.tv_sec == 0 && tv.tv_usec == 0) {
          return std::nullopt;
        }
        return std::chrono::seconds(tv.tv_sec) +
               std::chrono::microseconds(tv.tv_usec);
      });
}

} // namespace

error::result<void> Socket::set_read_timeout(
    std::optional<std::chrono::microseconds> timeout
) const {
  return set_socket_timeout(*this, SO_RCVTIMEO, timeout);
}

error::result<std::optional<std::chrono::microseconds>>
Socket::read_timeout() const {
  return get_socket_timeout(*this, SO_RCVTIMEO);
}

error::result<void> Socket::set_write_timeout(
    std::optional<std::chrono::microseconds> timeout
) const {
  return set_socket_timeout(*this, SO_SNDTIMEO, timeout);
}

error::result<std::optional<std::chrono::microseconds>>
Socket::write_timeout() const {
  return get_socket_timeout(*this, SO_SNDTIMEO);
}

} // namespace net
