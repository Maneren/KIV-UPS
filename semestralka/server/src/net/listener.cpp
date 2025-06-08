#include <fcntl.h>
#include <net/exception.h>
#include <net/listener.h>
#include <sys/socket.h>

namespace net {

constexpr int BACKLOG = 32;

TcpListener::TcpListener(const Address &addr)
    : sock(Socket(addr, SOCK_STREAM)) {
  sock.setopts(SOL_SOCKET, SO_REUSEADDR, 1);

  sock.bind_to(addr);

  if (listen(sock.fd.fd, BACKLOG) < 0) {
    throw io_exception("failed to start listening to socket");
  }
}

std::tuple<TcpStream, Address> TcpListener::accept() const {
  sockaddr_storage storage{};
  auto len = static_cast<socklen_t>(sizeof(storage));

  auto sock = this->sock.accept(reinterpret_cast<sockaddr &>(storage), len);
  const auto addr = Address::from_sockaddr(storage, len);

  return {TcpStream(std::move(sock)), addr};
}

} // namespace net
