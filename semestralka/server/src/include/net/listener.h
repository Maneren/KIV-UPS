#pragma once

#include "net/address.h"
#include "net/socket.h"
#include "net/stream.h"
#include <memory>

namespace net {

class TcpListener {
  Socket sock;

public:
  TcpListener(const Address &addr);
  ~TcpListener() = default;

  inline Socket &socket() { return sock; }

  std::tuple<TcpStream, std::unique_ptr<Address>> accept();
};

} // namespace net
