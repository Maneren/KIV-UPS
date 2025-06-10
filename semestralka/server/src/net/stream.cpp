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
      .map(functional::BindConstructor<TcpStream>());
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
      .map(functional::BindConstructor<TcpStream>());
}

TcpStream::~TcpStream() = default;

TcpStream::TcpStream(TcpStream &&other) noexcept
    : sock(std::move(other.sock)) {}

TcpStream &TcpStream::operator=(TcpStream &&other) noexcept {
  this->sock = std::move(other.sock);
  return *this;
}

error::result<ssize_t> TcpStream::read(std::span<std::byte> buf) const {
  return error::from_os(sock.read(buf.data(), buf.size()));
}

error::result<ssize_t> TcpStream::write(const std::span<const std::byte> buf
) const {
  return error::from_os(sock.write(buf.data(), buf.size()));
}

error::result<ssize_t>
TcpStream::recv(const std::span<std::byte> buf, int flags) const {
  return error::from_os(sock.recv(buf.data(), buf.size(), flags));
}

error::result<ssize_t>
TcpStream::send(const std::span<const std::byte> buf, int flags) const {
  return error::from_os(sock.send(buf.data(), buf.size(), flags));
}

} // namespace net
