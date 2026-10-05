#pragma once

#include <cstdint>
#include <net/error.h>
#include <net/ip_addr.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <tuple>
#include <utils/match.h>
#include <variant>
#include <vector>

namespace net {

struct SocketAddrV4 {
  constexpr static int FAMILY = Ipv4Addr::FAMILY;

  Ipv4Addr ip_;
  uint16_t port_;

  constexpr SocketAddrV4(Ipv4Addr ip, uint16_t port) : ip_(ip), port_(port) {}

  [[nodiscard]] static constexpr int family() { return Ipv4Addr::FAMILY; }

  [[nodiscard]] constexpr Ipv4Addr ip() const { return ip_; }
  void set_ip(Ipv4Addr ip) { ip_ = ip; }

  [[nodiscard]] constexpr uint16_t port() const { return port_; }
  void set_port(uint16_t port) { port_ = port; }

  constexpr bool operator==(const SocketAddrV4 &other) const = default;

  [[nodiscard]] sockaddr_in to_sockaddr() const;

  static error::result<SocketAddrV4>
  from_sockaddr(const sockaddr_storage &storage, socklen_t len);

  // Parses "127.0.0.1:80"; the port is required.
  static error::result<SocketAddrV4> from_string(const std::string &str);
};

struct SocketAddrV6 {
  constexpr static int FAMILY = Ipv6Addr::FAMILY;

  Ipv6Addr ip_;
  uint16_t port_;
  uint32_t flowinfo_;
  uint32_t scope_id_;

  constexpr SocketAddrV6(
      Ipv6Addr ip, uint16_t port, uint32_t flowinfo = 0, uint32_t scope_id = 0
  )
      : ip_(ip), port_(port), flowinfo_(flowinfo), scope_id_(scope_id) {}

  [[nodiscard]] static constexpr int family() { return Ipv6Addr::FAMILY; }

  [[nodiscard]] constexpr Ipv6Addr ip() const { return ip_; }
  void set_ip(Ipv6Addr ip) { ip_ = ip; }

  [[nodiscard]] constexpr uint16_t port() const { return port_; }
  void set_port(uint16_t port) { port_ = port; }

  [[nodiscard]] constexpr uint32_t flowinfo() const { return flowinfo_; }
  void set_flowinfo(uint32_t flowinfo) { flowinfo_ = flowinfo; }

  [[nodiscard]] constexpr uint32_t scope_id() const { return scope_id_; }
  void set_scope_id(uint32_t scope_id) { scope_id_ = scope_id; }

  constexpr bool operator==(const SocketAddrV6 &other) const = default;

  [[nodiscard]] sockaddr_in6 to_sockaddr() const;

  static error::result<SocketAddrV6>
  from_sockaddr(const sockaddr_storage &storage, socklen_t len);

  // Parses "[::1]:80" (also "[fe80::1%1]:80" with a numeric scope id);
  // the port is required.
  static error::result<SocketAddrV6> from_string(const std::string &str);
};

struct SocketAddr {
  // Implicit converting ctors are intentional: SocketAddrV4/V6 convert to
  // SocketAddr ergonomically
  // NOLINTNEXTLINE(*explicit-constructor)
  SocketAddr(SocketAddrV4 addr) : inner(addr) {}
  // NOLINTNEXTLINE(*explicit-constructor)
  SocketAddr(SocketAddrV6 addr) : inner(addr) {}

  SocketAddr(const IpAddr &ip, uint16_t port)
      : inner(SocketAddrV4(Ipv4Addr(), port)) {
    match::match(
        ip.inner,
        [port, this](const Ipv4Addr &v4) { inner = SocketAddrV4(v4, port); },
        [port, this](const Ipv6Addr &v6) { inner = SocketAddrV6(v6, port); }
    );
  }

  [[nodiscard]] IpAddr ip() const {
    return match::match(
        inner,
        [](const SocketAddrV4 &v4) { return IpAddr(v4.ip()); },
        [](const SocketAddrV6 &v6) { return IpAddr(v6.ip()); }
    );
  }
  void set_ip(const IpAddr &ip) {
    match::match(
        inner,
        [&ip](SocketAddrV4 &v4) {
          if (const auto *v = std::get_if<Ipv4Addr>(&ip.inner)) {
            v4.set_ip(*v);
          }
        },
        [&ip](SocketAddrV6 &v6) {
          if (const auto *v = std::get_if<Ipv6Addr>(&ip.inner)) {
            v6.set_ip(*v);
          }
        }
    );
  }

  [[nodiscard]] uint16_t port() const {
    return match::match(
        inner,
        [](const SocketAddrV4 &v4) { return v4.port(); },
        [](const SocketAddrV6 &v6) { return v6.port(); }
    );
  }
  void set_port(uint16_t port) {
    match::match(
        inner,
        [port](SocketAddrV4 &v4) { v4.set_port(port); },
        [port](SocketAddrV6 &v6) { v6.set_port(port); }
    );
  }

  [[nodiscard]] constexpr int family() const {
    return match::match(
        inner,
        [](const SocketAddrV4 &) { return SocketAddrV4::family(); },
        [](const SocketAddrV6 &) { return SocketAddrV6::family(); }
    );
  }

  [[nodiscard]] constexpr bool is_ipv4() const {
    return std::holds_alternative<SocketAddrV4>(inner);
  }
  [[nodiscard]] constexpr bool is_ipv6() const {
    return std::holds_alternative<SocketAddrV6>(inner);
  }

  [[nodiscard]] constexpr bool
  operator==(const SocketAddr &other) const = default;

  static error::result<SocketAddr> from_string(const std::string &str);

  /// Resolve a host + port via getaddrinfo (numeric or DNS name).
  static error::result<std::vector<SocketAddr>>
  resolve(const std::string &host, uint16_t port);

  static error::result<SocketAddr>
  from_sockaddr(const sockaddr_storage &storage, socklen_t len);

  union sockaddr_union {
    sockaddr_in ipv4;
    sockaddr_in6 ipv6;
  };

  [[nodiscard]] std::tuple<sockaddr_union, socklen_t> to_sockaddr() const;

private:
  std::variant<SocketAddrV4, SocketAddrV6> inner;

  friend std::formatter<SocketAddr>;
};

} // namespace net

template <> struct std::formatter<net::SocketAddrV4> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    return std::format_to(ctx.out(), "{}:{}", obj.ip(), obj.port());
  }
};

template <> struct std::formatter<net::SocketAddrV6> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    if (obj.scope_id() != 0) {
      return std::format_to(
          ctx.out(), "[{}%{}]:{}", obj.ip(), obj.scope_id(), obj.port()
      );
    }
    return std::format_to(ctx.out(), "[{}]:{}", obj.ip(), obj.port());
  }
};

template <> struct std::formatter<net::SocketAddr> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    return match::match(
        obj.inner,
        [&ctx](const net::SocketAddrV4 &addr) {
          return std::format_to(ctx.out(), "{}", addr);
        },
        [&ctx](const net::SocketAddrV6 &addr) {
          return std::format_to(ctx.out(), "{}", addr);
        }
    );
  }
};
