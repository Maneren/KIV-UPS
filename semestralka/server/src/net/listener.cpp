#include "net/exception.h"
#include <fcntl.h>
#include <net/listener.h>
#include <sys/socket.h>

namespace net {

constexpr int BACKLOG = 32;

TcpListener::TcpListener(const Address &addr)
    : sock(Socket(addr, SOCK_STREAM)) {
  sock.setopts(SOL_SOCKET, SO_REUSEADDR, 1);

  sock.bind_to(addr);

  if (listen(sock.fd.fd, BACKLOG) < 0) {
    throw IoException("failed to start listening to socket");
  }
}

std::tuple<Socket, std::unique_ptr<Address>> TcpListener::accept() {
  sockaddr_storage storage;
  auto len = static_cast<socklen_t>(sizeof(storage));

  const auto sock =
      this->sock.accept(reinterpret_cast<sockaddr *>(&storage), &len);
  const auto addr = IPv4Address::from_sockaddr(storage, len);

  return std::make_tuple(std::move(sock), std::make_unique<IPv4Address>(addr));
}

} // namespace net
