#include <fcntl.h>
#include <net/listener.h>
#include <sys/socket.h>

namespace net {

constexpr int BACKLOG = 32;

error::result<TcpListener> TcpListener::bind(const Address &addr) {
  auto sock = Socket::create(addr, SOCK_STREAM).value();
  sock.setopts(SOL_SOCKET, SO_REUSEADDR, 1);

  if (const auto result = sock.bind_to(addr); !result) {
    return tl::make_unexpected(result.error());
  }

  return error::from_os(listen(sock.fd.fd, BACKLOG)).map([&](auto) {
    return TcpListener(std::move(sock));
  });
}

error::result<std::tuple<TcpStream, Address>> TcpListener::accept() const {
  sockaddr_storage storage{};
  auto len = static_cast<socklen_t>(sizeof(storage));

  auto sock = this->sock.accept(reinterpret_cast<sockaddr &>(storage), len);

  if (!sock) {
    return tl::make_unexpected(sock.error());
  }

  const auto addr = Address::from_sockaddr(storage, len);

  if (!addr) {
    return tl::make_unexpected(addr.error());
  }

  return std::make_tuple(TcpStream(std::move(sock.value())), addr.value());
}

} // namespace net
