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
#include <utility>
#include <utils/match.h>
#include <variant>

namespace net {

struct Ipv4Addr {
  constexpr static size_t BYTES = 4;
  constexpr static int FAMILY = AF_INET;

  using octets_t = std::array<uint8_t, BYTES>;

  constexpr Ipv4Addr() noexcept = default;
  constexpr Ipv4Addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d) noexcept
      : octets{a, b, c, d} {}
  explicit constexpr Ipv4Addr(octets_t octets) noexcept : octets(octets) {}
  explicit Ipv4Addr(uint32_t addr) noexcept;

  constexpr bool operator==(const Ipv4Addr &other) const noexcept = default;

  [[nodiscard]] static constexpr int family() noexcept { return FAMILY; }

  [[nodiscard]] constexpr std::array<uint8_t, BYTES>
  to_octets() const noexcept {
    return octets;
  }

  [[nodiscard]] uint32_t to_bits() const noexcept;
  [[nodiscard]] static Ipv4Addr from_bits(uint32_t bits) noexcept;

  [[nodiscard]] constexpr bool is_loopback() const noexcept {
    return octets.front() == 127;
  }
  [[nodiscard]] constexpr bool is_unspecified() const noexcept {
    return octets == octets_t{0, 0, 0, 0};
  }
  [[nodiscard]] constexpr bool is_broadcast() const noexcept {
    return octets == octets_t{255, 255, 255, 255};
  }
  [[nodiscard]] constexpr bool is_multicast() const noexcept {
    const auto [a, _b, _c, _d] = octets;
    return a >= 224 && a <= 239;
  }
  [[nodiscard]] constexpr bool is_private() const noexcept {
    const auto [a, b, _c, _d] = octets;
    return (a == 10) || (a == 172 && b >= 16 && b <= 31) ||
           (a == 192 && b == 168);
  }

  static Ipv4Addr localhost() noexcept;
  static Ipv4Addr unspecified() noexcept;
  static Ipv4Addr broadcast() noexcept;

  static error::result<Ipv4Addr> from_string(std::string_view str);

private:
  octets_t octets{};

  friend std::formatter<Ipv4Addr>;
  friend struct SocketAddrV4;
};

struct Ipv6Addr {
  constexpr static size_t BYTES = 16;
  constexpr static size_t SEGMENTS = BYTES / sizeof(uint16_t);
  constexpr static int FAMILY = AF_INET6;

  using octets_t = std::array<uint8_t, BYTES>;

  Ipv6Addr() noexcept = default;
  explicit Ipv6Addr(octets_t octets) noexcept;
  explicit Ipv6Addr(const uint8_t octets[BYTES]) noexcept;
  static Ipv6Addr
  from_segments(std::array<uint16_t, SEGMENTS> segments) noexcept;

  constexpr bool operator==(const Ipv6Addr &other) const noexcept = default;

  [[nodiscard]] static constexpr int family() noexcept { return FAMILY; }

  [[nodiscard]] constexpr std::array<uint8_t, BYTES>
  to_octets() const noexcept {
    return octets;
  }

  [[nodiscard]] std::array<uint16_t, SEGMENTS> segments() const noexcept;

  [[nodiscard]] bool is_loopback() const noexcept;
  [[nodiscard]] bool is_unspecified() const noexcept;
  [[nodiscard]] bool is_multicast() const noexcept;
  static Ipv6Addr localhost() noexcept;
  static Ipv6Addr unspecified() noexcept;

  // Parses a bare literal, e.g. "::1". Bracketed `[ip]:port` forms
  // belong to `SocketAddrV6::from_string`.
  static error::result<Ipv6Addr> from_string(std::string_view str);

private:
  octets_t octets{};

  friend std::formatter<Ipv6Addr>;
};

struct IpAddr {
  // Implicit converting ctors are intentional: IpAddrV4/V6 convert to IpAddr
  // ergonomically
  // NOLINTNEXTLINE(*explicit-constructor)
  IpAddr(Ipv4Addr addr) noexcept : inner(addr) {}
  // NOLINTNEXTLINE(*explicit-constructor)
  IpAddr(Ipv6Addr addr) noexcept : inner(addr) {}

  template <typename... Fs>
  constexpr decltype(auto) visit(Fs &&...fs) const noexcept(
      noexcept(match::match(inner, std::forward<Fs>(fs)...))
  ) {
    return match::match(inner, std::forward<Fs>(fs)...);
  }

  template <typename... Fs>
  constexpr decltype(auto) visit(Fs &&...fs) noexcept(
      noexcept(match::match(inner, std::forward<Fs>(fs)...))
  ) {
    return match::match(inner, std::forward<Fs>(fs)...);
  }

  [[nodiscard]] constexpr int family() const {
    return visit(
        [](const Ipv4Addr &) { return Ipv4Addr::FAMILY; },
        [](const Ipv6Addr &) { return Ipv6Addr::FAMILY; }
    );
  }

  [[nodiscard]] constexpr bool is_ipv4() const noexcept {
    return std::holds_alternative<Ipv4Addr>(inner);
  }
  [[nodiscard]] constexpr bool is_ipv6() const noexcept {
    return std::holds_alternative<Ipv6Addr>(inner);
  }

  [[nodiscard]] bool is_loopback() const;
  [[nodiscard]] bool is_unspecified() const;
  [[nodiscard]] bool is_multicast() const;

  [[nodiscard]] constexpr bool operator==(const IpAddr &other) const = default;

  static error::result<IpAddr> from_string(std::string_view str);

private:
  std::variant<Ipv4Addr, Ipv6Addr> inner;
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
    return obj.visit(
        [&ctx](const net::Ipv4Addr &addr) {
          return std::format_to(ctx.out(), "{}", addr);
        },
        [&ctx](const net::Ipv6Addr &addr) {
          return std::format_to(ctx.out(), "{}", addr);
        }
    );
  }
};
