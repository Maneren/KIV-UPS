#pragma once

#include "net/socket.h"
#include <span>

namespace net {

class TcpStream {
  Socket sock;

public:
  TcpStream(Socket &&sock) : sock(std::move(sock)) {}

  ~TcpStream() = default;

  TcpStream(const TcpStream &) = delete;
  TcpStream(TcpStream &&) = default;
  TcpStream &operator=(const TcpStream &) = delete;
  TcpStream &operator=(TcpStream &&) = default;

  int read(const std::span<std::byte> buf) const;
  int write(const std::span<const std::byte> buf) const;

  int recv(const std::span<std::byte> buf, int flags = 0) const;
  int send(const std::span<const std::byte> buf, int flags = 0) const;
};

} // namespace net
