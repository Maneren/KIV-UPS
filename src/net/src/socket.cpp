#include <cerrno>
#include <ctime>
#include <fcntl.h>
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
  requires std::is_integral_v<std::invoke_result_t<F>>
auto retry_on_eintr(F &&fn) -> error::result<std::invoke_result_t<F>> {
  while (true) {
    if (auto ret = std::forward<F>(fn)(); ret >= 0) {
      return static_cast<std::make_unsigned_t<std::invoke_result_t<F>>>(ret);
    }
    if (errno == EINTR) {
      continue;
    }
    return std::unexpected(error::Os{});
  }
}

// Enables O_NONBLOCK for the duration of a connect-with-timeout and
// restores the original flags afterwards. A pre-existing O_NONBLOCK is
// left alone instead of assuming blocking mode.
class NonblockingGuard {
public:
  explicit NonblockingGuard(int fd) : fd(fd) {
    // There is no other way to do this
    // NOLINTNEXTLINE(*cppcoreguidelines-pro-type-vararg)
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
      error = errno;
      return;
    }
    if ((flags & O_NONBLOCK) != 0) {
      return;
    }
    // NOLINTNEXTLINE(*cppcoreguidelines-pro-type-vararg)
    if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
      error = errno;
      return;
    }
    saved = flags;
    active = true;
  }

  NonblockingGuard(const NonblockingGuard &) = delete;
  NonblockingGuard &operator=(const NonblockingGuard &) = delete;

  NonblockingGuard(NonblockingGuard &&) = delete;
  NonblockingGuard &operator=(NonblockingGuard &&) = delete;

  ~NonblockingGuard() {
    if (active) {
      // Best effort: failures are reported through restore() instead.
      // NOLINTNEXTLINE(*cppcoreguidelines-pro-type-vararg)
      ::fcntl(fd, F_SETFL, saved);
    }
  }

  [[nodiscard]] error::result<void> status() const {
    if (error != 0) {
      return std::unexpected(error::Os{error});
    }
    return {};
  }

  error::result<void> restore() {
    if (!active) {
      return {};
    }
    // NOLINTNEXTLINE(*cppcoreguidelines-pro-type-vararg)
    if (::fcntl(fd, F_SETFL, saved) == -1) {
      return std::unexpected(error::Os{});
    }
    active = false;
    return {};
  }

private:
  int fd;
  int saved = 0;
  int error = 0;
  bool active = false;
};

struct timespec to_timespec(std::chrono::nanoseconds timeout) {
  const auto secs = std::chrono::duration_cast<std::chrono::seconds>(timeout);
  const auto nsecs = timeout - secs;
  return {
      .tv_sec = secs.count(),
      .tv_nsec = nsecs.count(),
  };
}

error::IoError timed_out_error() {
  return error::Simple{error::ErrorKind::TimedOut, "Connection timed out"};
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

  return error::from_os(fd).transform([](int raw) {
    return Socket(FileDescriptor(raw));
  });
}

error::result<Socket> Socket::create(const SocketAddr &addr, int type) {
  return create(addr.family(), type);
}

error::result<Socket> Socket::duplicate() const {
  return fd.duplicate().transform(functional::Constructor<Socket>());
}

error::result<void> Socket::bind_to(const SocketAddr &addr) const {
  const auto [sockaddr, len] = addr.to_sockaddr();

  return error::from_os(bind(raw_fd(), &sockaddr.sa, len))
      .transform(functional::drop);
}

error::result<Socket>
Socket::accept(sockaddr_union &sockaddr, socklen_t &len, int flags) const {
  return retry_on_eintr([&] {
           return accept4(raw_fd(), &sockaddr.sa, &len, flags | SOCK_CLOEXEC);
         })
      .transform([](int raw) { return Socket(FileDescriptor(raw)); });
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

    return std::unexpected(error::Os{errno});
  }
}

error::result<void> Socket::connect_timeout(
    const SocketAddr &addr, std::chrono::nanoseconds timeout
) const {
  if (timeout <= std::chrono::nanoseconds::zero()) {
    return std::unexpected(
        error::Simple{
            error::ErrorKind::InvalidInput, "Timeout must be positive"
        }
    );
  }

  NonblockingGuard nonblocking(raw_fd());
  if (auto status = nonblocking.status(); !status) {
    return std::unexpected(std::move(status).error());
  }

  const auto [storage, len] = addr.to_sockaddr();

  if (::connect(raw_fd(), &storage.sa, len) == 0 || errno == EISCONN) {
    return nonblocking.restore();
  }

  const int connect_errno = errno;
  if (connect_errno != EINPROGRESS && connect_errno != EINTR) {
    return std::unexpected(error::Os{connect_errno});
  }

  struct pollfd pfd{.fd = raw_fd(), .events = POLLOUT, .revents = 0};

  const auto deadline = std::chrono::steady_clock::now() + timeout;

  while (true) {
    const auto remaining = deadline - std::chrono::steady_clock::now();
    if (remaining <= std::chrono::steady_clock::duration::zero()) {
      return std::unexpected(timed_out_error());
    }

    const struct timespec wait = to_timespec(
        std::chrono::duration_cast<std::chrono::nanoseconds>(remaining)
    );
    const int poll_result = ppoll(&pfd, 1, &wait, nullptr);

    if (poll_result == -1) {
      if (errno == EINTR) {
        continue;
      }
      return std::unexpected(error::Os{errno});
    }

    if (poll_result == 0) {
      return std::unexpected(timed_out_error());
    }

    // A writable socket may still carry a pending async error, so
    // always consult SO_ERROR (not only on POLLERR/POLLHUP).
    auto error_result = take_error();
    if (!error_result) {
      return std::unexpected(std::move(error_result).error());
    }

    if (auto pending = std::move(*error_result); pending) {
      return std::unexpected(std::move(*pending));
    }

    return nonblocking.restore();
  }
}

error::result<std::optional<error::IoError>> Socket::take_error() const {
  auto result = getopt<int>(SOL_SOCKET, SO_ERROR);

  if (!result) {
    return std::unexpected(std::move(result).error());
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
    return std::unexpected(error::Os{errno});
  }
  const int updated =
      nonblocking ? (current | O_NONBLOCK) : (current & ~O_NONBLOCK);
  if (updated == current) {
    return {};
  }
  // NOLINTNEXTLINE(*cppcoreguidelines-pro-type-vararg)
  return error::from_os(::fcntl(raw_fd(), F_SETFL, updated))
      .transform(functional::drop);
}

error::result<size_t> Socket::read(void *buf, const size_t len) const {
  return retry_on_eintr([&] { return ::read(raw_fd(), buf, len); });
}

error::result<size_t> Socket::write(const void *buf, const size_t len) const {
  return retry_on_eintr([&] { return ::write(raw_fd(), buf, len); });
}

error::result<size_t>
Socket::recv(void *buf, const size_t len, int flags) const {
  return retry_on_eintr([&] { return ::recv(raw_fd(), buf, len, flags); });
}

error::result<size_t>
Socket::send(const void *buf, const size_t len, int flags) const {
  return retry_on_eintr([&] { return ::send(raw_fd(), buf, len, flags); });
}

namespace {

error::result<SocketAddr>
addr_from_fd(int fd, int (*getter)(int, sockaddr *, socklen_t *)) {
  sockaddr_union storage{};
  auto len = sockaddr_union::SIZE;
  if (getter(fd, &storage.sa, &len) == -1) {
    return std::unexpected(error::Os{errno});
  }
  return SocketAddr::from_sockaddr(storage, len);
}

} // namespace

error::result<SocketAddr> Socket::local_addr() const {
  return addr_from_fd(raw_fd(), ::getsockname);
}

error::result<SocketAddr> Socket::peer_addr() const {
  return addr_from_fd(raw_fd(), ::getpeername);
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
  default:
    std::unreachable();
  }
  return error::from_os(::shutdown(raw_fd(), flag)).transform(functional::drop);
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
  return getopt<int>(IPPROTO_TCP, TCP_NODELAY).transform([](int opt) {
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
  return setopt(IPPROTO_IPV6, IPV6_UNICAST_HOPS, opt);
}

error::result<uint32_t> Socket::ttl() const {
  if (const auto addr = local_addr(); addr) {
    if (addr->family() == AF_INET6) {
      return getopt<int>(IPPROTO_IPV6, IPV6_UNICAST_HOPS)
          .transform([](int opt) { return static_cast<uint32_t>(opt); });
    }
    return getopt<int>(IPPROTO_IP, IP_TTL).transform([](int opt) {
      return static_cast<uint32_t>(opt);
    });
  }
  // Unbound: try v4, then v6.
  if (auto v4 = getopt<int>(IPPROTO_IP, IP_TTL); v4) {
    return static_cast<uint32_t>(*v4);
  }
  return getopt<int>(IPPROTO_IPV6, IPV6_UNICAST_HOPS).transform([](int opt) {
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
      return std::unexpected(
          error::Simple{
              error::ErrorKind::InvalidInput, "Timeout must be non-negative"
          }
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
      .transform(
          [](struct timeval tv) -> std::optional<std::chrono::microseconds> {
            if (tv.tv_sec == 0 && tv.tv_usec == 0) {
              return std::nullopt;
            }
            return std::chrono::seconds(tv.tv_sec) +
                   std::chrono::microseconds(tv.tv_usec);
          }
      );
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
