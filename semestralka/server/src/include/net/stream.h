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

  int read(std::span<std::byte> buf) const;
  int write(const std::span<std::byte> buf) const;

  // int recv(std::span<std::byte> buf) const { return read(buf); }
  // int send(const std::span<std::byte> buf) const { return write(buf); }
};

} // namespace net
