#pragma once

#include <net/socket.h>
#include <net/socket_addr.h>
#include <net/stream.h>
#include <optional>
#include <ranges>
#include <tuple>
#include <utility>

namespace net {

class TcpListener {
  Socket sock;

public:
  explicit TcpListener(Socket &&sock) : sock(std::move(sock)) {}
  static error::result<TcpListener> bind(const SocketAddr &addr);
  static error::result<TcpListener>
  bind_with_backlog(const SocketAddr &addr, int backlog);

  Socket &socket() { return sock; }
  [[nodiscard]] const Socket &socket() const { return sock; }

  [[nodiscard]] error::result<std::tuple<TcpStream, SocketAddr>> accept() const;

  [[nodiscard]] error::result<SocketAddr> local_addr() const {
    return sock.local_addr();
  }
  [[nodiscard]] error::result<TcpListener> duplicate() const {
    return sock.duplicate().map([](Socket s) {
      return TcpListener(std::move(s));
    });
  }
  [[nodiscard]] error::result<void> set_nonblocking(bool nonblocking) const {
    return sock.set_nonblocking(nonblocking);
  }
  [[nodiscard]] error::result<std::optional<error::IoError>>
  take_error() const {
    return sock.take_error();
  }
  [[nodiscard]] error::result<void> set_ttl(uint32_t ttl) const {
    return sock.set_ttl(ttl);
  }
  [[nodiscard]] error::result<uint32_t> ttl() const { return sock.ttl(); }

  [[nodiscard]] auto incoming() const {
    return std::ranges::transform_view(std::ranges::iota_view(0), [&](auto) {
      return this->accept();
    });
  }
};

} // namespace net
