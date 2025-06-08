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

  TcpStream(const Address &addr);
  TcpStream(const Address &addr, std::chrono::microseconds timeout);

  int read(const std::span<std::byte> buf) const;
  int write(const std::span<const std::byte> buf) const;

  int recv(const std::span<std::byte> buf, int flags = 0) const;
  int send(const std::span<const std::byte> buf, int flags = 0) const;

  const Socket &socket() const { return sock; }
};

} // namespace net
