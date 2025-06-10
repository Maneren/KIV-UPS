#pragma once

#include <net/address.h>
#include <net/socket.h>
#include <net/stream.h>
#include <ranges>

namespace net {

class TcpListener {
  Socket sock;

public:
  TcpListener(Socket &&sock) : sock(std::move(sock)) {}
  static error::result<TcpListener> bind(const Address &addr);

  Socket &socket() { return sock; }

  [[nodiscard]] error::result<std::tuple<TcpStream, Address>> accept() const;

  [[nodiscard]] auto incoming() const {
    return std::ranges::transform_view(std::ranges::iota_view(0), [&](auto) {
      return this->accept();
    });
  }
};

} // namespace net
