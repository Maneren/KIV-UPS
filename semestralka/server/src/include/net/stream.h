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

  [[nodiscard]] error::result<ssize_t> read(std::span<std::byte> buf) const;
  [[nodiscard]] error::result<ssize_t> write(std::span<const std::byte> buf
  ) const;

  [[nodiscard]] error::result<ssize_t>
  recv(std::span<std::byte> buf, int flags = 0) const;
  [[nodiscard]] error::result<ssize_t>
  send(std::span<const std::byte> buf, int flags = 0) const;

  [[nodiscard]] const Socket &socket() const { return sock; }
};

} // namespace net
