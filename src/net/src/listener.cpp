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
      return std::move(*result);
    }

    last_error = std::move(result).error();
  }

  if (last_error) {
    return tl::make_unexpected(std::move(*last_error));
  }

  return tl::make_unexpected(
      error::Simple{
          error::ErrorKind::InvalidInput, "could not bind to any address"
      }
  );
}

error::result<TcpListener>
TcpListener::bind_with_backlog(const SocketAddr &addr, int backlog) {
  return Socket::create(addr, SOCK_STREAM)
      .and_then([&addr, backlog](Socket sock) -> error::result<TcpListener> {
        return sock.setopt(SOL_SOCKET, SO_REUSEPORT, 1)
            .and_then([&sock, &addr]() { return sock.bind_to(addr); })
            .and_then([&sock, backlog]() {
              return error::from_os(listen(sock.raw_fd(), backlog));
            })
            .map([&sock](int) { return TcpListener(std::move(sock)); });
      });
}

error::result<std::tuple<TcpStream, SocketAddr>> TcpListener::accept() const {
  sockaddr_union storage{};
  auto len = sockaddr_union::SIZE;

  auto sock = this->sock.accept(storage, len);
  if (!sock) {
    return tl::make_unexpected(std::move(sock).error());
  }

  auto addr = SocketAddr::from_sockaddr(storage, len);
  if (!addr) {
    return tl::make_unexpected(std::move(addr).error());
  }

  return std::make_tuple(TcpStream(std::move(*sock)), *addr);
}

error::result<SocketAddr> TcpListener::local_addr() const {
  return sock.local_addr();
}

error::result<TcpListener> TcpListener::duplicate() const {
  return sock.duplicate().map(functional::Constructor<TcpListener>());
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
