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
  explicit TcpListener(Socket &&sock);
  static error::result<TcpListener> bind(const SocketAddr &addr);
  static error::result<TcpListener>
  bind_with_backlog(const SocketAddr &addr, int backlog);

  Socket &socket() noexcept { return sock; }
  [[nodiscard]] const Socket &socket() const noexcept { return sock; }

  [[nodiscard]] error::result<std::tuple<TcpStream, SocketAddr>> accept() const;

  [[nodiscard]] error::result<SocketAddr> local_addr() const;
  [[nodiscard]] error::result<TcpListener> duplicate() const;
  [[nodiscard]] error::result<void> set_nonblocking(bool nonblocking) const;
  [[nodiscard]] error::result<std::optional<error::IoError>> take_error() const;
  [[nodiscard]] error::result<void> set_ttl(uint32_t ttl) const;
  [[nodiscard]] error::result<uint32_t> ttl() const;

  [[nodiscard]] auto incoming() const {
    return std::ranges::transform_view(std::ranges::iota_view(0), [&](auto) {
      return this->accept();
    });
  }
};

} // namespace net
