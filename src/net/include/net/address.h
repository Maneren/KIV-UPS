#pragma once

#include <arpa/inet.h>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <format>
#include <net/error.h>
#include <netinet/in.h>
#include <string>
#include <tuple>
#include <utils/match.h>
#include <variant>
#include <vector>

namespace net {

struct IPv4Address {
  constexpr static size_t BYTES = 4;
  constexpr static int FAMILY = AF_INET;

  std::array<uint8_t, BYTES> octets{};
  uint16_t port;

  IPv4Address(uint32_t addr, uint16_t port = 0) : port(port) {
    auto in_net_endian = htonl(addr);
    std::memcpy(octets.data(), &in_net_endian, BYTES);
  }
  IPv4Address(std::array<uint8_t, BYTES> octets, uint16_t port)
      : octets(octets), port(port) {}

  constexpr bool operator==(const IPv4Address &other) const {
    return octets == other.octets && port == other.port;
  }

  [[nodiscard]] static constexpr int family() { return FAMILY; }

  [[nodiscard]] uint32_t to_uint32() const {
    uint32_t net_endian = 0;
    std::memcpy(&net_endian, octets.data(), BYTES);
    return ntohl(net_endian);
  }

  [[nodiscard]] constexpr bool is_loopback() const { return octets[0] == 127; }
  [[nodiscard]] constexpr bool is_unspecified() const {
    return octets == std::array<uint8_t, BYTES>{0, 0, 0, 0};
  }
  [[nodiscard]] constexpr bool is_broadcast() const {
    return octets == std::array<uint8_t, BYTES>{255, 255, 255, 255};
  }
  [[nodiscard]] constexpr bool is_multicast() const {
    return octets[0] >= 224 && octets[0] <= 239;
  }
  [[nodiscard]] constexpr bool is_private() const {
    return (octets[0] == 10) ||
           (octets[0] == 172 && octets[1] >= 16 && octets[1] <= 31) ||
           (octets[0] == 192 && octets[1] == 168);
  }

  static IPv4Address localhost(uint16_t port = 0) {
    return IPv4Address({127, 0, 0, 1}, port);
  }
  static IPv4Address unspecified(uint16_t port = 0) {
    return IPv4Address({0, 0, 0, 0}, port);
  }
  static IPv4Address broadcast(uint16_t port = 0) {
    return IPv4Address({255, 255, 255, 255}, port);
  }

  [[nodiscard]] sockaddr_in to_sockaddr() const;

  static error::result<IPv4Address>
  from_sockaddr(const sockaddr_storage &storage, socklen_t len);

  static error::result<IPv4Address> from_string(const std::string &str);
};

struct IPv6Address {
  constexpr static size_t BYTES = 16;
  constexpr static int FAMILY = AF_INET6;

  std::array<uint8_t, BYTES> octets{};
  uint16_t port;
  uint32_t flowinfo;
  uint32_t scopeid;

  IPv6Address(
      std::array<uint8_t, BYTES> octets,
      uint16_t port = 0,
      uint32_t flowinfo = 0,
      uint32_t scopeid = 0
  )
      : octets(octets), port(port), flowinfo(flowinfo), scopeid(scopeid) {}
  IPv6Address(
      // NOLINTNEXTLINE(modernize-avoid-c-arrays)
      const uint8_t octets[BYTES],
      uint16_t port = 0,
      uint32_t flowinfo = 0,
      uint32_t scopeid = 0
  )
      : port(port), flowinfo(flowinfo), scopeid(scopeid) {
    std::memcpy(this->octets.data(), octets, BYTES);
  }

  constexpr bool operator==(const IPv6Address &other) const {
    return octets == other.octets && port == other.port &&
           flowinfo == other.flowinfo && scopeid == other.scopeid;
  }

  [[nodiscard]] static constexpr int family() { return FAMILY; }

  [[nodiscard]] bool is_loopback() const;
  [[nodiscard]] bool is_unspecified() const;
  [[nodiscard]] bool is_multicast() const;

  static IPv6Address localhost(uint16_t port = 0);
  static IPv6Address unspecified(uint16_t port = 0);

  [[nodiscard]] sockaddr_in6 to_sockaddr() const;

  static error::result<IPv6Address>
  from_sockaddr(const sockaddr_storage &storage, socklen_t len);

  static error::result<IPv6Address> from_string(const std::string &str);
};

struct Address {
  Address(IPv4Address addr) : inner(addr) {}
  Address(IPv6Address addr) : inner(addr) {}

  std::variant<IPv4Address, IPv6Address> inner;

  [[nodiscard]] constexpr int family() const {
    return match::match(
        inner,
        [](const IPv4Address &) { return IPv4Address::FAMILY; },
        [](const IPv6Address &) { return IPv6Address::FAMILY; }
    );
  }

  [[nodiscard]] constexpr bool is_ipv4() const {
    return std::holds_alternative<IPv4Address>(inner);
  }
  [[nodiscard]] constexpr bool is_ipv6() const {
    return std::holds_alternative<IPv6Address>(inner);
  }

  [[nodiscard]] constexpr bool operator==(const Address &other) const {
    return inner == other.inner;
  }

  static error::result<Address> from_string(const std::string &str);

  /// Resolve a host + port via getaddrinfo (numeric or DNS name).
  static error::result<std::vector<Address>>
  resolve(const std::string &host, uint16_t port);

  static error::result<Address>
  from_sockaddr(const sockaddr_storage &storage, socklen_t len);

  union sockaddr_union {
    sockaddr_in ipv4;
    sockaddr_in6 ipv6;
  };

  [[nodiscard]] std::tuple<sockaddr_union, socklen_t> to_sockaddr() const;

  [[nodiscard]] uint16_t port() const;
  void set_port(uint16_t port);
};

} // namespace net

template <> struct std::formatter<net::IPv4Address> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    return std::format_to(
        ctx.out(),
        "{}.{}.{}.{}:{}",
        obj.octets[0],
        obj.octets[1],
        obj.octets[2],
        obj.octets[3],
        obj.port
    );
  }
};

template <> struct std::formatter<net::IPv6Address> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    std::string buffer(INET6_ADDRSTRLEN, '\0');

    const auto cp = obj.to_sockaddr();

    const auto result = inet_ntop(
        obj.family(),
        &cp.sin6_addr,
        buffer.data(),
        static_cast<socklen_t>(buffer.size())
    );

    if (result == nullptr) {
      throw std::runtime_error("inet_ntop failed to stringify address");
    }

    return std::format_to(ctx.out(), "[{}]:{}", buffer.c_str(), obj.port);
  }
};

template <> struct std::formatter<net::Address> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    return match::match(
        obj.inner,
        [&ctx](const net::IPv4Address &addr) {
          return std::format_to(ctx.out(), "{}", addr);
        },
        [&ctx](const net::IPv6Address &addr) {
          return std::format_to(ctx.out(), "{}", addr);
        }
    );
  }
};
