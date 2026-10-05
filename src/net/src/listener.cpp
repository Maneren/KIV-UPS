#include <net/listener.h>
#include <sys/socket.h>
#include <tl/expected.hpp>

namespace net {

constexpr int BACKLOG = 32;

error::result<TcpListener> TcpListener::bind(const SocketAddr &addr) {
  return bind_with_backlog(addr, BACKLOG);
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

} // namespace net
