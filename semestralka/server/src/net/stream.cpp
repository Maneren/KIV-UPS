#include <net/stream.h>

namespace net {

int TcpStream::read(std::span<std::byte> buf) const {
  const auto len = buf.size();
  const auto result = sock.read(buf.data(), len);
  if (result < 0) {
    throw IoException("failed to read from socket");
  }
  return result;
}

int TcpStream::write(const std::span<std::byte> buf) const {
  const auto len = buf.size();
  const auto result = sock.write(buf.data(), len);
  if (result < 0) {
    throw IoException("failed to write to socket");
  }
  return result;
}

} // namespace net
