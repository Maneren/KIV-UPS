#pragma once

#include <array>
#include <cstring>
#include <format>
#include <netinet/in.h>
#include <string>

namespace net {

struct Address {
  Address() {}
  virtual ~Address() {}
  virtual int family() const = 0;
  virtual sockaddr_in to_sockaddr() const = 0;
};

struct IPv4Address : public Address {
  std::array<uint8_t, 4> octets;
  uint16_t port;

  constexpr static size_t BYTES = 4;

  IPv4Address() {}
  IPv4Address(std::array<uint8_t, BYTES> octets, uint16_t port)
      : octets(octets), port(port) {}

  ~IPv4Address() override = default;

  int family() const override { return AF_INET; }

  sockaddr_in to_sockaddr() const override;

  static IPv4Address
  from_sockaddr(const sockaddr_storage &storage, socklen_t len);
};

} // namespace net

template <> struct std::formatter<net::IPv4Address> {
  constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  auto format(const net::IPv4Address &obj, std::format_context &ctx) const {
    return std::format_to(ctx.out(), "{}.{}.{}.{}:{}", obj.octets[0], obj.octets[1], obj.octets[2], obj.octets[3], obj.port);
  }
};

template <> struct std::formatter<net::Address> {
  constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  auto format(const net::Address &obj, std::format_context &ctx) const {
    if (dynamic_cast<const net::IPv4Address *>(&obj)) {
      return std::format_to(ctx.out(), "{}", dynamic_cast<const net::IPv4Address &>(obj));
    }
    return std::format_to(ctx.out(), "unknown address");
  }
};
  

