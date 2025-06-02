#include <net/stream.h>

namespace net {

int TcpStream::read(std::span<std::byte> buf) const {
  const auto len = buf.size();
  const auto result = sock.read(buf.data(), len);
  if (result < 0) {
    throw io_exception("failed to read from socket");
  }
  return result;
}

int TcpStream::write(const std::span<const std::byte> buf) const {
  const auto len = buf.size();
  const auto result = sock.write(buf.data(), len);
  if (result < 0) {
    throw io_exception("failed to write to socket");
  }
  return result;
}

int TcpStream::recv(const std::span<std::byte> buf, int flags) const {
  const auto len = buf.size();
  const auto result = sock.recv(buf.data(), len, flags);
  if (result < 0) {
    throw io_exception("failed to recv from socket");
  }
  return result;
}

int TcpStream::send(const std::span<const std::byte> buf, int flags) const {
  const auto len = buf.size();
  const auto result = sock.send(buf.data(), len, flags);
  if (result < 0) {
    throw io_exception("failed to send to socket");
  }
  return result;
}

} // namespace net
