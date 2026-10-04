#pragma once

#include <chrono>
#include <net/socket.h>
#include <span>

namespace net {

class TcpStream {
  Socket sock;

public:
  TcpStream(Socket &&sock);

  ~TcpStream();

  TcpStream(const TcpStream &) = delete;
  TcpStream(TcpStream &&other) noexcept;
  TcpStream &operator=(const TcpStream &) = delete;
  TcpStream &operator=(TcpStream &&other) noexcept;

  static error::result<TcpStream> connect(const Address &addr);
  static error::result<TcpStream>
  connect_timeout(const Address &addr, std::chrono::microseconds timeout);

  template <typename T>
  [[nodiscard]] error::result<ssize_t> read(std::span<T> buf) const {
    const auto byte_buf = std::as_writable_bytes(buf);
    return error::from_os(sock.read(byte_buf.data(), byte_buf.size()));
  };
  template <typename T>
  [[nodiscard]] error::result<ssize_t> write(std::span<T> buf) const {
    const auto byte_buf = std::as_bytes(buf);
    return error::from_os(sock.write(byte_buf.data(), byte_buf.size()));
  };

  template <typename T>
  [[nodiscard]] error::result<ssize_t>
  recv(std::span<T> buf, int flags = 0) const {
    const auto byte_buf = std::as_writable_bytes(buf);
    return error::from_os(sock.recv(byte_buf.data(), byte_buf.size(), flags));
  };
  template <typename T>
  [[nodiscard]] error::result<ssize_t>
  send(std::span<T> buf, int flags = 0) const {
    const auto byte_buf = std::as_bytes(buf);
    return error::from_os(sock.send(byte_buf.data(), byte_buf.size(), flags));
  };

  [[nodiscard]] const Socket &socket() const { return sock; }
};

} // namespace net
