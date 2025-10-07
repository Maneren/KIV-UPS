#pragma once

#include <arpa/inet.h>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <format>
#include <net/error.h>
#include <netinet/in.h>
#include <tuple>
#include <utils/match.h>
#include <variant>

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

  static error::result<Address>
  from_sockaddr(sockaddr_storage &storage, size_t len);

  union sockaddr_union {
    sockaddr_in ipv4;
    sockaddr_in6 ipv6;
  };

  [[nodiscard]] std::tuple<sockaddr_union, int> to_sockaddr() const;

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
