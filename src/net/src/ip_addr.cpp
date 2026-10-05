#include <arpa/inet.h>
#include <net/ip_addr.h>
#include <netinet/in.h>
#include <string>

namespace net {

// Ipv4Addr

error::result<Ipv4Addr> Ipv4Addr::from_string(const std::string &str) {
  if (str.empty() || str.find(':') != std::string::npos) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid IPv4 address: {}", str
        )
    );
  }

  struct in_addr addr{};
  if (inet_pton(AF_INET, str.c_str(), &addr) != 1) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid IPv4 address: {}", str
        )
    );
  }

  return Ipv4Addr{ntohl(addr.s_addr)};
}

// Ipv6Addr

bool Ipv6Addr::is_loopback() const {
  static constexpr std::array<uint8_t, BYTES> loopback{
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
  };
  return octets == loopback;
}

bool Ipv6Addr::is_unspecified() const {
  static constexpr std::array<uint8_t, BYTES> unspecified{};
  return octets == unspecified;
}

bool Ipv6Addr::is_multicast() const { return octets[0] == 0xff; }

Ipv6Addr Ipv6Addr::localhost() {
  return Ipv6Addr(
      std::array<uint8_t, BYTES>{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}
  );
}

Ipv6Addr Ipv6Addr::unspecified() {
  return Ipv6Addr(std::array<uint8_t, BYTES>{});
}

error::result<Ipv6Addr> Ipv6Addr::from_string(const std::string &str) {
  if (str.empty() || str.starts_with('[')) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid IPv6 address: {}", str
        )
    );
  }

  struct in6_addr addr{};
  if (inet_pton(AF_INET6, str.c_str(), &addr) != 1) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid IPv6 address: {}", str
        )
    );
  }

  return Ipv6Addr{static_cast<uint8_t *>(addr.s6_addr)};
}

// IpAddr

error::result<IpAddr> IpAddr::from_string(const std::string &str) {
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
