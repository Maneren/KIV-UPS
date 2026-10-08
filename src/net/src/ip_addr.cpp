#include <arpa/inet.h>
#include <cstring>
#include <net/ip_addr.h>
#include <netinet/in.h>

namespace net {

// Ipv4Addr

Ipv4Addr::Ipv4Addr(uint32_t addr) noexcept {
  const auto in_net_endian = htonl(addr);
  std::memcpy(octets_.data(), &in_net_endian, BYTES);
}

uint32_t Ipv4Addr::to_bits() const noexcept {
  uint32_t net_endian = 0;
  std::memcpy(&net_endian, octets_.data(), BYTES);
  return ntohl(net_endian);
}

Ipv4Addr Ipv4Addr::from_bits(uint32_t bits) noexcept { return Ipv4Addr(bits); }

Ipv4Addr Ipv4Addr::localhost() noexcept { return {127, 0, 0, 1}; }
Ipv4Addr Ipv4Addr::unspecified() noexcept { return {0, 0, 0, 0}; }
Ipv4Addr Ipv4Addr::broadcast() noexcept { return {255, 255, 255, 255}; }

error::result<Ipv4Addr> Ipv4Addr::from_string(std::string_view str) {
  static constexpr auto invalid_ipv4 = [](std::string_view str) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid IPv4 address: {}", str
        )
    );
  };

  if (str.empty() || str.size() >= INET_ADDRSTRLEN) {
    return invalid_ipv4(str);
  }

  std::array<char, INET_ADDRSTRLEN> null_terminated{};
  std::memcpy(null_terminated.data(), str.data(), str.size());

  struct in_addr addr{};
  if (inet_pton(AF_INET, null_terminated.data(), &addr) != 1) {
    return invalid_ipv4(str);
  }

  return Ipv4Addr{ntohl(addr.s_addr)};
}

// Ipv6Addr

Ipv6Addr::Ipv6Addr(const uint8_t octets[BYTES]) noexcept {
  std::memcpy(this->octets.data(), octets, BYTES);
}

constexpr Ipv6Addr
Ipv6Addr::from_segments(std::array<uint16_t, SEGMENTS> segments) noexcept {
  Ipv6Addr addr;
  for (size_t i = 0; i < SEGMENTS; ++i) {
    const auto segment = segments[i];
    addr.octets[2 * i] =
        static_cast<uint8_t>(static_cast<unsigned>(segment) >> 8U);
    addr.octets[(2 * i) + 1] = static_cast<uint8_t>(segment);
  }
  return addr;
}

std::array<uint16_t, Ipv6Addr::SEGMENTS> Ipv6Addr::segments() const noexcept {
  std::array<uint16_t, SEGMENTS> segs{};
  for (size_t i = 0; i < SEGMENTS; ++i) {
    segs[i] = static_cast<uint16_t>(
        (static_cast<unsigned>(octets[2 * i]) << 8U) |
        static_cast<unsigned>(octets[(2 * i) + 1])
    );
  }
  return segs;
}

constexpr bool Ipv6Addr::is_loopback() const noexcept {
  static constexpr octets_t loopback{
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
  };
  return octets == loopback;
}

constexpr bool Ipv6Addr::is_unspecified() const noexcept {
  static constexpr octets_t unspecified{};
  return octets == unspecified;
}

constexpr bool Ipv6Addr::is_multicast() const noexcept {
  return octets.front() == 0xff;
}

Ipv6Addr Ipv6Addr::localhost() noexcept {
  octets_t octets{};
  octets.back() = 1;
  return Ipv6Addr(octets);
}

Ipv6Addr Ipv6Addr::unspecified() noexcept { return {}; }

error::result<Ipv6Addr> Ipv6Addr::from_string(std::string_view str) {
  if (str.empty() || str.starts_with('[') || str.size() >= INET6_ADDRSTRLEN) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid IPv6 address: {}", str
        )
    );
  }

  std::array<char, INET6_ADDRSTRLEN> null_terminated{};
  std::memcpy(null_terminated.data(), str.data(), str.size());

  struct in6_addr addr{};
  if (inet_pton(AF_INET6, null_terminated.data(), &addr) != 1) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid IPv6 address: {}", str
        )
    );
  }

  octets_t octets{};
  std::memcpy(octets.data(), static_cast<std::uint8_t *>(addr.s6_addr), BYTES);
  return Ipv6Addr{octets};
}

// IpAddr

bool IpAddr::is_loopback() const {
  return visit([](const auto &a) { return a.is_loopback(); });
}

bool IpAddr::is_unspecified() const {
  return visit([](const auto &a) { return a.is_unspecified(); });
}

bool IpAddr::is_multicast() const {
  return visit([](const auto &a) { return a.is_multicast(); });
}

error::result<IpAddr> IpAddr::from_string(std::string_view str) {
  // ':' only appears in IPv6 literals; dispatch on shape so only one
  // branch is ever constructed.
  if (str.contains(':')) {
    if (auto v6 = Ipv6Addr::from_string(str); v6) {
      return IpAddr(*v6);
    }
  } else {
    if (auto v4 = Ipv4Addr::from_string(str); v4) {
      return IpAddr(*v4);
    }
  }
  return tl::make_unexpected(
      error::SimpleMessage(
          error::ErrorKind::InvalidInput, "Invalid IP address: {}", str
      )
  );
}

} // namespace net
