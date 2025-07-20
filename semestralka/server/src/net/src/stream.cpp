#include <net/stream.h>

namespace net {

TcpStream::TcpStream(Socket &&sock) : sock(std::move(sock)) {};

error::result<TcpStream> TcpStream::connect(const Address &addr) {
  return Socket::create(addr, SOCK_STREAM)
      .and_then([&addr](auto &&sock) {
        return sock.connect(addr).map([&]() {
          return std::forward<Socket>(sock);
        });
      })
      .map(functional::Constructor<TcpStream>());
}
error::result<TcpStream> TcpStream::connect_timeout(
    const Address &addr, std::chrono::microseconds timeout
) {
  return Socket::create(addr, SOCK_STREAM)
      .and_then([&addr, timeout](auto &&sock) {
        return sock.connect_timeout(addr, timeout).map([&]() {
          return std::forward<Socket>(sock);
        });
      })
      .map(functional::Constructor<TcpStream>());
}

TcpStream::~TcpStream() = default;

TcpStream::TcpStream(TcpStream &&other) noexcept
    : sock(std::move(other.sock)) {}

TcpStream &TcpStream::operator=(TcpStream &&other) noexcept {
  this->sock = std::move(other.sock);
  return *this;
}

} // namespace net
