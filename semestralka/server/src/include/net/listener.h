#pragma once

#include <net/address.h>
#include <net/socket.h>
#include <net/stream.h>

namespace net {

class TcpListener {
  Socket sock;

public:
  TcpListener(const Address &addr);
  ~TcpListener() = default;

  inline Socket &socket() { return sock; }

  std::tuple<TcpStream, Address> accept() const;
};

} // namespace net
