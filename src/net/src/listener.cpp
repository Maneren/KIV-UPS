#include <net/listener.h>
#include <sys/socket.h>
#include <tl/expected.hpp>
#include <utility>

namespace net {

constexpr int BACKLOG = 32;

TcpListener::TcpListener(Socket &&sock) : sock(std::move(sock)) {}

error::result<TcpListener> TcpListener::bind(const SocketAddr &addr) {
  return bind_with_backlog(addr, BACKLOG);
}
error::result<TcpListener>
TcpListener::bind(std::span<const SocketAddr> addrs) {
  std::optional<net::error::IoError> last_error;
  for (const auto &addr : addrs) {
    auto result = bind_with_backlog(addr, BACKLOG);
    if (result) {
      return result;
    }

    last_error = result.error();
  }

  if (last_error) {
    return tl::make_unexpected(*last_error);
  }

  return tl::make_unexpected(
      error::SimpleMessage(
          error::ErrorKind::InvalidInput, "could not bind to any address"
      )
  );
}

error::result<TcpListener>
TcpListener::bind_with_backlog(const SocketAddr &addr, int backlog) {
  return Socket::create(addr, SOCK_STREAM)
      .and_then([&addr, backlog](Socket sock) -> error::result<TcpListener> {
        if (const auto result = sock.setopts(SOL_SOCKET, SO_REUSEADDR, 1);
            !result) {
          return tl::make_unexpected(result.error());
        }

        if (const auto result = sock.bind_to(addr); !result) {
          return tl::make_unexpected(result.error());
        }

        if (const auto result = error::from_os(listen(sock.raw_fd(), backlog));
            !result) {
          return tl::make_unexpected(result.error());
        }

        return TcpListener(std::move(sock));
      });
}

error::result<std::tuple<TcpStream, SocketAddr>> TcpListener::accept() const {
  sockaddr_union storage{};
  auto len = sockaddr_union::SIZE;

  auto sock = this->sock.accept(storage, len);

  if (!sock) {
    return tl::make_unexpected(sock.error());
  }

  const auto addr = SocketAddr::from_sockaddr(storage, len);

  if (!addr) {
    return tl::make_unexpected(addr.error());
  }

  return std::make_tuple(TcpStream(std::move(sock.value())), addr.value());
}

error::result<SocketAddr> TcpListener::local_addr() const {
  return sock.local_addr();
}

error::result<TcpListener> TcpListener::duplicate() const {
  return sock.duplicate().map([](Socket s) {
    return TcpListener(std::move(s));
  });
}

error::result<void> TcpListener::set_nonblocking(bool nonblocking) const {
  return sock.set_nonblocking(nonblocking);
}

error::result<std::optional<error::IoError>> TcpListener::take_error() const {
  return sock.take_error();
}

error::result<void> TcpListener::set_ttl(uint32_t ttl) const {
  return sock.set_ttl(ttl);
}

error::result<uint32_t> TcpListener::ttl() const { return sock.ttl(); }

} // namespace net
