#pragma once

#include <net/socket.h>
#include <net/socket_addr.h>
#include <net/stream.h>
#include <optional>
#include <ranges>
#include <span>
#include <tuple>

namespace net {

class TcpListener {
  Socket sock;

public:
  explicit TcpListener(Socket &&sock);
  static error::result<TcpListener> bind(const SocketAddr &addr);
  static error::result<TcpListener> bind(std::span<const SocketAddr> addrs);
  static error::result<TcpListener>
  bind_with_backlog(const SocketAddr &addr, int backlog);

  TcpListener(const TcpListener &) = delete;
  TcpListener(TcpListener &&) noexcept = default;
  TcpListener &operator=(const TcpListener &) = delete;
  TcpListener &operator=(TcpListener &&) noexcept = default;
  ~TcpListener() = default;

  [[nodiscard]] const Socket &socket() const noexcept { return sock; }

  [[nodiscard]] error::result<std::tuple<TcpStream, SocketAddr>> accept() const;

  [[nodiscard]] error::result<SocketAddr> local_addr() const;
  [[nodiscard]] error::result<TcpListener> duplicate() const;
  [[nodiscard]] error::result<void> set_nonblocking(bool nonblocking) const;
  [[nodiscard]] error::result<std::optional<error::IoError>> take_error() const;
  [[nodiscard]] error::result<void> set_ttl(uint32_t ttl) const;
  [[nodiscard]] error::result<uint32_t> ttl() const;

  [[nodiscard]] auto incoming() const {
    return std::ranges::transform_view(std::ranges::iota_view(0), [this](auto) {
      return this->accept();
    });
  }
};

} // namespace net
