#pragma once

#include <arpa/inet.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <net/error.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <utils/match.h>
#include <variant>

namespace net {

struct Ipv4Addr {
  constexpr static size_t BYTES = 4;
  constexpr static int FAMILY = AF_INET;

  constexpr Ipv4Addr() = default;
  constexpr Ipv4Addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
      : octets{a, b, c, d} {}
  explicit constexpr Ipv4Addr(std::array<uint8_t, BYTES> octets)
      : octets(octets) {}
  explicit Ipv4Addr(uint32_t addr) {
    auto in_net_endian = htonl(addr);
    std::memcpy(octets.data(), &in_net_endian, BYTES);
  }

  constexpr bool operator==(const Ipv4Addr &other) const = default;

  [[nodiscard]] static constexpr int family() { return FAMILY; }

  [[nodiscard]] constexpr std::array<uint8_t, BYTES> to_octets() const {
    return octets;
  }

  [[nodiscard]] uint32_t to_bits() const {
    uint32_t net_endian = 0;
    std::memcpy(&net_endian, octets.data(), BYTES);
    return ntohl(net_endian);
  }
  [[nodiscard]] static Ipv4Addr from_bits(uint32_t bits) {
    // NOLINTNEXTLINE(modernize-return-braced-init-list): braced elision
    // would require a non-explicit converting ctor.
    return Ipv4Addr(bits);
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

  static Ipv4Addr localhost() { return {127, 0, 0, 1}; }
  static Ipv4Addr unspecified() { return {0, 0, 0, 0}; }
  static Ipv4Addr broadcast() { return {255, 255, 255, 255}; }

  static error::result<Ipv4Addr> from_string(const std::string &str);

private:
  std::array<uint8_t, BYTES> octets{};

  friend std::formatter<Ipv4Addr>;
  friend struct SocketAddrV4;
};

struct Ipv6Addr {
  constexpr static size_t BYTES = 16;
  constexpr static int FAMILY = AF_INET6;

  std::array<uint8_t, BYTES> octets{};

  Ipv6Addr() = default;
  explicit Ipv6Addr(std::array<uint8_t, BYTES> octets) : octets(octets) {}
  explicit Ipv6Addr(
      // NOLINTNEXTLINE(modernize-avoid-c-arrays)
      const uint8_t octets[BYTES]
  ) {
    std::memcpy(this->octets.data(), octets, BYTES);
  }
  static Ipv6Addr from_segments(std::array<uint16_t, 8> segments) {
    Ipv6Addr addr;
    for (size_t i = 0; i < 8; ++i) {
      addr.octets[2 * i] = static_cast<uint8_t>(segments[i] >> 8);
      addr.octets[(2 * i) + 1] = static_cast<uint8_t>(segments[i] & 0xff);
    }
    return addr;
  }

  constexpr bool operator==(const Ipv6Addr &other) const = default;

  [[nodiscard]] static constexpr int family() { return FAMILY; }

  [[nodiscard]] constexpr std::array<uint8_t, BYTES> to_octets() const {
    return octets;
  }

  [[nodiscard]] std::array<uint16_t, 8> segments() const {
    std::array<uint16_t, 8> segs{};
    for (size_t i = 0; i < 8; ++i) {
      segs[i] = static_cast<uint16_t>(
          (static_cast<uint16_t>(octets[2 * i]) << 8) | octets[(2 * i) + 1]
      );
    }
    return segs;
  }

  [[nodiscard]] bool is_loopback() const;
  [[nodiscard]] bool is_unspecified() const;
  [[nodiscard]] bool is_multicast() const;

  static Ipv6Addr localhost();
  static Ipv6Addr unspecified();

  // Parses a bare literal, e.g. "::1". Bracketed `[ip]:port` forms
  // belong to `SocketAddrV6::from_string`.
  static error::result<Ipv6Addr> from_string(const std::string &str);
};

struct IpAddr {
  explicit IpAddr(Ipv4Addr addr) : inner(addr) {}
  explicit IpAddr(Ipv6Addr addr) : inner(addr) {}

  std::variant<Ipv4Addr, Ipv6Addr> inner;

  [[nodiscard]] constexpr int family() const {
    return match::match(
        inner,
        [](const Ipv4Addr &) { return Ipv4Addr::FAMILY; },
        [](const Ipv6Addr &) { return Ipv6Addr::FAMILY; }
    );
  }

  [[nodiscard]] constexpr bool is_ipv4() const {
    return std::holds_alternative<Ipv4Addr>(inner);
  }
  [[nodiscard]] constexpr bool is_ipv6() const {
    return std::holds_alternative<Ipv6Addr>(inner);
  }

  [[nodiscard]] bool is_loopback() const {
    return match::match(
        inner,
        [](const Ipv4Addr &a) { return a.is_loopback(); },
        [](const Ipv6Addr &a) { return a.is_loopback(); }
    );
  }
  [[nodiscard]] bool is_unspecified() const {
    return match::match(
        inner,
        [](const Ipv4Addr &a) { return a.is_unspecified(); },
        [](const Ipv6Addr &a) { return a.is_unspecified(); }
    );
  }
  [[nodiscard]] bool is_multicast() const {
    return match::match(
        inner,
        [](const Ipv4Addr &a) { return a.is_multicast(); },
        [](const Ipv6Addr &a) { return a.is_multicast(); }
    );
  }

  [[nodiscard]] constexpr bool operator==(const IpAddr &other) const = default;

  static error::result<IpAddr> from_string(const std::string &str);
};

} // namespace net

template <> struct std::formatter<net::Ipv4Addr> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    return std::format_to(
        ctx.out(),
        "{}.{}.{}.{}",
        obj.octets[0],
        obj.octets[1],
        obj.octets[2],
        obj.octets[3]
    );
  }
};

template <> struct std::formatter<net::Ipv6Addr> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    std::string buffer(INET6_ADDRSTRLEN, '\0');

    sockaddr_in6 raw{};
    std::memset(&raw, 0, sizeof(raw));
    std::memcpy(
        &raw.sin6_addr.s6_addr, obj.octets.data(), net::Ipv6Addr::BYTES
    );

    const auto *const result = inet_ntop(
        net::Ipv6Addr::FAMILY,
        &raw.sin6_addr,
        buffer.data(),
        static_cast<socklen_t>(buffer.size())
    );

    if (result == nullptr) {
      throw std::runtime_error("inet_ntop failed to stringify address");
    }

    // c_str() truncates at the NUL written by inet_ntop; formatting
    // `buffer` directly would emit padding.
    // NOLINTNEXTLINE(readability-redundant-string-cstr)
    return std::format_to(ctx.out(), "{}", buffer.c_str());
  }
};

template <> struct std::formatter<net::IpAddr> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    return match::match(
        obj.inner,
        [&ctx](const net::Ipv4Addr &addr) {
          return std::format_to(ctx.out(), "{}", addr);
        },
        [&ctx](const net::Ipv6Addr &addr) {
          return std::format_to(ctx.out(), "{}", addr);
        }
    );
  }
};
