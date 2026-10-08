#include <arpa/inet.h>
#include <cstring>
#include <net/ip_addr.h>
#include <netinet/in.h>
#include <utils/ranges.h>

namespace net {

// Ipv4Addr

Ipv4Addr::Ipv4Addr(uint32_t addr) noexcept {
  const auto in_net_endian = htonl(addr);
  std::memcpy(octets.data(), &in_net_endian, BYTES);
}

uint32_t Ipv4Addr::to_bits() const noexcept {
  uint32_t net_endian = 0;
  std::memcpy(&net_endian, octets.data(), BYTES);
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

  if (str.empty()) {
    return invalid_ipv4(str);
  }

  const std::string null_terminated{str};

  struct in_addr addr{};
  if (inet_pton(AF_INET, null_terminated.c_str(), &addr) != 1) {
    return invalid_ipv4(str);
  }

  return Ipv4Addr{ntohl(addr.s_addr)};
}

// Ipv6Addr

Ipv6Addr::Ipv6Addr(octets_t octets) noexcept : octets(octets) {}

Ipv6Addr::Ipv6Addr(const uint8_t octets[BYTES]) noexcept {
  std::memcpy(this->octets.data(), octets, BYTES);
}

Ipv6Addr
Ipv6Addr::from_segments(std::array<uint16_t, SEGMENTS> segments) noexcept {
  Ipv6Addr addr;
  for (const auto [i, segment] : utils::views::enumerate_uz(segments)) {
    addr.octets.at(2 * i) =
        static_cast<uint8_t>(static_cast<unsigned>(segment) >> 8U);
    addr.octets.at((2 * i) + 1) = static_cast<uint8_t>(segment);
  }
  return addr;
}

std::array<uint16_t, Ipv6Addr::SEGMENTS> Ipv6Addr::segments() const noexcept {
  std::array<uint16_t, SEGMENTS> segs{};
  for (size_t i = 0; i < SEGMENTS; ++i) {
    segs.at(i) = static_cast<uint16_t>(
        (static_cast<unsigned>(octets.at(2 * i)) << 8U) |
        static_cast<unsigned>(octets.at((2 * i) + 1))
    );
  }
  return segs;
}

bool Ipv6Addr::is_loopback() const noexcept {
  static constexpr std::array<uint8_t, BYTES> loopback{
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      1,
  };
  return octets == loopback;
}

bool Ipv6Addr::is_unspecified() const noexcept {
  static constexpr std::array<uint8_t, BYTES> unspecified{};
  return octets == unspecified;
}

bool Ipv6Addr::is_multicast() const noexcept { return octets.front() == 0xff; }

Ipv6Addr Ipv6Addr::localhost() noexcept {
  return Ipv6Addr(
      std::array<uint8_t, BYTES>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}
  );
}

Ipv6Addr Ipv6Addr::unspecified() noexcept {
  return Ipv6Addr(std::array<uint8_t, BYTES>{});
}

error::result<Ipv6Addr> Ipv6Addr::from_string(std::string_view str) {
  if (str.empty() || str.starts_with('[')) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid IPv6 address: {}", str
        )
    );
  }

  const std::string null_terminated{str};

  struct in6_addr addr{};
  if (inet_pton(AF_INET6, null_terminated.c_str(), &addr) != 1) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid IPv6 address: {}", str
        )
    );
  }

  return Ipv6Addr{static_cast<uint8_t *>(addr.s6_addr)};
}

// IpAddr

bool IpAddr::is_loopback() const {
  return visit(
      [](const Ipv4Addr &a) { return a.is_loopback(); },
      [](const Ipv6Addr &a) { return a.is_loopback(); }
  );
}

bool IpAddr::is_unspecified() const {
  return visit(
      [](const Ipv4Addr &a) { return a.is_unspecified(); },
      [](const Ipv6Addr &a) { return a.is_unspecified(); }
  );
}

bool IpAddr::is_multicast() const {
  return visit(
      [](const Ipv4Addr &a) { return a.is_multicast(); },
      [](const Ipv6Addr &a) { return a.is_multicast(); }
  );
}

error::result<IpAddr> IpAddr::from_string(std::string_view str) {
  if (const auto v4 = Ipv4Addr::from_string(str); v4) {
    return IpAddr(*v4);
  }
  if (const auto v6 = Ipv6Addr::from_string(str); v6) {
    return IpAddr(*v6);
  }
  return tl::make_unexpected(
      error::SimpleMessage(
          error::ErrorKind::InvalidInput, "Invalid IP address: {}", str
      )
  );
}

} // namespace net
