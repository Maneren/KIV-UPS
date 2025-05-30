#pragma once

#include "net/exception.h"
#include "utils/algebraic.h"
#include <arpa/inet.h>
#include <array>
#include <cstring>
#include <format>
#include <functional>
#include <netinet/in.h>
#include <string>
#include <tuple>
#include <type_traits>

namespace net {

enum class AddressType : int { IPV4 = AF_INET, IPV6 = AF_INET6 };

struct IPv4Address {
  constexpr static size_t BYTES = 4;
  static const AddressType type = AddressType::IPV4;

  std::array<uint8_t, BYTES> octets;
  uint16_t port;

  IPv4Address() {}
  IPv4Address(std::array<uint8_t, BYTES> octets, uint16_t port)
      : octets(octets), port(port) {}

  ~IPv4Address() = default;

  constexpr int family() const { return static_cast<int>(type); }

  sockaddr_in to_sockaddr() const;

  static IPv4Address
  from_sockaddr(const sockaddr_storage &storage, socklen_t len);
};

struct IPv6Address {
  constexpr static size_t BYTES = 16;
  static const AddressType type = AddressType::IPV6;

  std::array<uint8_t, BYTES> octets;
  uint16_t port;
  uint32_t flowinfo;
  uint32_t scopeid;

  IPv6Address() {}
  IPv6Address(std::array<uint8_t, BYTES> octets, uint16_t port)
      : octets(octets), port(port) {}

  ~IPv6Address() = default;

  constexpr int family() const { return static_cast<int>(type); }

  sockaddr_in6 to_sockaddr() const;

  static IPv6Address
  from_sockaddr(const sockaddr_storage &storage, socklen_t len);
};

struct Address {
  AddressType const type;
  Address(IPv4Address addr) : type(AddressType::IPV4), ipv4(addr) {}
  Address(IPv6Address addr) : type(AddressType::IPV6), ipv6(addr) {}
  union {
    IPv4Address ipv4;
    IPv6Address ipv6;
  };

  int family() const {
    switch (type) {
    case AddressType::IPV4:
      return ipv4.family();
    case AddressType::IPV6:
      return ipv6.family();
    }
  }

  static Address from_sockaddr(sockaddr_storage &storage, size_t len) {
    if (storage.ss_family == AF_INET) {
      return Address(IPv4Address::from_sockaddr(storage, len));
    } else if (storage.ss_family == AF_INET6) {
      return Address(IPv6Address::from_sockaddr(storage, len));
    } else {
      throw IoException("unrecognixed socket family");
    }
  }

  union sockaddr_union {
    sockaddr_in ipv4;
    sockaddr_in6 ipv6;
  };

  std::tuple<sockaddr_union, int> to_sockaddr() const {
    sockaddr_union storage;
    int len;
    switch (type) {
    case AddressType::IPV4:
      storage.ipv4 = ipv4.to_sockaddr();
      len = sizeof(sockaddr_in);
      break;
    case AddressType::IPV6:
      storage.ipv6 = ipv6.to_sockaddr();
      len = sizeof(sockaddr_in6);
      break;
    }
    return {storage, len};
  }
};

} // namespace net

template <> struct std::formatter<net::IPv4Address> {
  constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

  auto format(const net::IPv4Address &obj, std::format_context &ctx) const {
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
  constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

  auto format(const net::IPv6Address &obj, std::format_context &ctx) const {
    constexpr std::string_view longest_addr =
        "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff";

    std::string buffer('\0', longest_addr.size());

    const auto result = inet_ntop(
        obj.family(), obj.octets.data(), buffer.data(), buffer.size()
    );

    if (result == nullptr) {
      throw std::runtime_error("inet_ntop failed to stringify address");
    }

    buffer.resize(strlen(buffer.data()));

    return std::format_to(ctx.out(), "[{}]:{}", buffer, obj.port);
  }
};

template <> struct std::formatter<net::Address> {
  constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

  auto format(const net::Address &obj, std::format_context &ctx) const {
    return algebraic::match(
        obj,
        &net::Address::ipv4,
        [&ctx](const net::IPv4Address &addr) {
          return std::format_to(ctx.out(), "{}", addr);
        },
        &net::Address::ipv6,
        [&ctx](const net::IPv6Address &addr) {
          return std::format_to(ctx.out(), "{}", addr);
        }
    );
  }
};
