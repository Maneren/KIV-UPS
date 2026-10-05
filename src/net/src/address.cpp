#include <charconv>
#include <format>
#include <net/address.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <utils/functional.h>

namespace net {

namespace {

error::result<uint16_t> parse_port(const std::string_view port_part) {
  if (port_part.empty()) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Missing port number"
        )
    );
  }
  unsigned long value = 0;
  const auto [ptr, ec] =
      std::from_chars(port_part.begin(), port_part.end(), value);
  if (ec != std::errc{} || ptr != port_part.end() || value > UINT16_MAX) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid port number: {}", port_part
        )
    );
  }
  return static_cast<uint16_t>(value);
}

} // namespace

sockaddr_in net::IPv4Address::to_sockaddr() const {
  sockaddr_in addr_in{};
  std::memset(&addr_in, 0, sizeof(addr_in));
  addr_in.sin_family = AF_INET;
  addr_in.sin_port = htons(port);

  static_assert(sizeof(addr_in.sin_addr.s_addr) == BYTES);
  std::memcpy(&addr_in.sin_addr.s_addr, octets.data(), BYTES);

  return addr_in;
}

error::result<IPv4Address>
IPv4Address::from_sockaddr(const sockaddr_storage &storage, socklen_t len) {
  if (len < sizeof(sockaddr_in)) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Invalid address length: {} < {}",
            len,
            sizeof(sockaddr_in)
        )
    );
  }

  const auto *const addr_in =
      reinterpret_cast<const sockaddr_in *const>(&storage);

  if (addr_in->sin_family != FAMILY) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Invalid address family for IPv4: {}",
            addr_in->sin_family
        )
    );
  }

  static_assert(sizeof(addr_in->sin_addr.s_addr) == BYTES);
  return IPv4Address{ntohl(addr_in->sin_addr.s_addr), ntohs(addr_in->sin_port)};
}

error::result<IPv4Address> IPv4Address::from_string(const std::string &str) {
  if (str.empty()) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid IPv4 address: {}", str
        )
    );
  }

  const size_t colon_pos = str.find(':');
  const std::string ip_part =
      (colon_pos == std::string::npos) ? str : str.substr(0, colon_pos);
  const bool has_port = colon_pos != std::string::npos;
  const std::string port_part = has_port ? str.substr(colon_pos + 1) : "";

  struct in_addr addr{};
  if (inet_pton(AF_INET, ip_part.c_str(), &addr) != 1) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid IPv4 address: {}", str
        )
    );
  }

  uint16_t port = 0;
  if (has_port) {
    const auto parsed = parse_port(port_part);
    if (!parsed) {
      return tl::make_unexpected(parsed.error());
    }
    port = *parsed;
  }

  return IPv4Address{ntohl(addr.s_addr), port};
}

sockaddr_in6 net::IPv6Address::to_sockaddr() const {
  sockaddr_in6 addr_in6{};
  std::memset(&addr_in6, 0, sizeof(addr_in6));
  addr_in6.sin6_family = AF_INET6;
  addr_in6.sin6_port = htons(port);
  addr_in6.sin6_flowinfo = htonl(flowinfo);
  addr_in6.sin6_scope_id = scopeid;

  static_assert(sizeof(addr_in6.sin6_addr.s6_addr) == BYTES);
  std::memcpy(&addr_in6.sin6_addr.s6_addr, octets.data(), BYTES);

  return addr_in6;
}

error::result<IPv6Address>
IPv6Address::from_sockaddr(const sockaddr_storage &storage, socklen_t len) {
  if (len < sizeof(sockaddr_in6)) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Invalid IPv6 address length: {} < {}",
            len,
            sizeof(sockaddr_in6)
        )
    );
  }

  const auto *const addr_in6 =
      reinterpret_cast<const sockaddr_in6 *const>(&storage);

  if (addr_in6->sin6_family != FAMILY) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Invalid address family for IPv6: {}",
            addr_in6->sin6_family
        )
    );
  }

  static_assert(sizeof(addr_in6->sin6_addr.s6_addr) == BYTES);
  return IPv6Address{
      static_cast<const uint8_t *>(addr_in6->sin6_addr.s6_addr),
      ntohs(addr_in6->sin6_port),
      ntohl(addr_in6->sin6_flowinfo),
      addr_in6->sin6_scope_id
  };
}

error::result<IPv6Address> IPv6Address::from_string(const std::string &str) {
  if (str.empty()) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid IPv6 address: {}", str
        )
    );
  }

  std::string ip_part;
  std::string port_part;
  bool has_port = false;

  if (str.starts_with('[')) {
    const size_t bracket_pos = str.find(']');

    if (bracket_pos == std::string::npos) {
      return tl::make_unexpected(
          error::SimpleMessage(
              error::ErrorKind::InvalidInput, "Invalid IPv6 address: {}", str
          )
      );
    }

    ip_part = str.substr(1, bracket_pos - 1);
    const std::string rest = str.substr(bracket_pos + 1);
    if (!rest.empty()) {
      if (!rest.starts_with(':')) {
        return tl::make_unexpected(
            error::SimpleMessage(
                error::ErrorKind::InvalidInput, "Invalid IPv6 address: {}", str
            )
        );
      }
      has_port = true;
      port_part = rest.substr(1);
    }
  } else {
    // Without brackets the whole string is the IP literal; a port
    // requires bracket notation.
    ip_part = str;
  }

  struct in6_addr addr{};
  if (inet_pton(AF_INET6, ip_part.c_str(), &addr) != 1) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid IPv6 address: {}", str
        )
    );
  }

  uint16_t port = 0;
  if (has_port) {
    const auto parsed = parse_port(port_part);
    if (!parsed) {
      return tl::make_unexpected(parsed.error());
    }
    port = *parsed;
  }

  return IPv6Address{static_cast<uint8_t *>(addr.s6_addr), port};
}

error::result<Address>
Address::from_sockaddr(const sockaddr_storage &storage, socklen_t len) {
  switch (storage.ss_family) {
  case IPv4Address::FAMILY:
    return IPv4Address::from_sockaddr(storage, len)
        .map(functional::Constructor<Address>());
  case IPv6Address::FAMILY:
    return IPv6Address::from_sockaddr(storage, len)
        .map(functional::Constructor<Address>());
  default:
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Unrecognized socket family: {}",
            storage.ss_family
        )
    );
  }
}

std::tuple<Address::sockaddr_union, socklen_t> Address::to_sockaddr() const {
  return match::match(
      inner,
      [](const IPv4Address &ipv4) {
        return std::make_tuple(
            sockaddr_union{.ipv4 = ipv4.to_sockaddr()},
            static_cast<socklen_t>(sizeof(sockaddr_in))
        );
      },
      [](const IPv6Address &ipv6) {
        return std::make_tuple(
            sockaddr_union{.ipv6 = ipv6.to_sockaddr()},
            static_cast<socklen_t>(sizeof(sockaddr_in6))
        );
      }
  );
}

uint16_t Address::port() const {
  return match::match(
      inner,
      [](const IPv4Address &ipv4) { return ipv4.port; },
      [](const IPv6Address &ipv6) { return ipv6.port; }
  );
}

void Address::set_port(uint16_t port) {
  match::match(
      inner,
      [&port](IPv4Address &ipv4) { ipv4.port = port; },
      [&port](IPv6Address &ipv6) { ipv6.port = port; }
  );
}

bool IPv6Address::is_loopback() const {
  static constexpr std::array<uint8_t, BYTES> loopback{
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
  };
  return octets == loopback;
}

bool IPv6Address::is_unspecified() const {
  static constexpr std::array<uint8_t, BYTES> unspecified{};
  return octets == unspecified;
}

bool IPv6Address::is_multicast() const { return octets[0] == 0xff; }

IPv6Address IPv6Address::localhost(uint16_t port) {
  return IPv6Address(
      std::array<uint8_t, BYTES>{
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
      },
      port
  );
}

IPv6Address IPv6Address::unspecified(uint16_t port) {
  return IPv6Address(std::array<uint8_t, BYTES>{}, port);
}

error::result<Address> Address::from_string(const std::string &str) {
  if (const auto v4 = IPv4Address::from_string(str); v4) {
    return Address(*v4);
  }
  if (const auto v6 = IPv6Address::from_string(str); v6) {
    return Address(*v6);
  }
  return tl::make_unexpected(
      error::SimpleMessage(
          error::ErrorKind::InvalidInput, "Invalid socket address: {}", str
      )
  );
}

error::result<std::vector<Address>>
Address::resolve(const std::string &host, uint16_t port) {
  const std::string service = std::to_string(port);

  struct addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *list = nullptr;
  const int code = getaddrinfo(host.c_str(), service.c_str(), &hints, &list);
  if (code != 0) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Failed to resolve {}: {}",
            host,
            gai_strerror(code)
        )
    );
  }

  std::vector<Address> out;
  for (const struct addrinfo *ai = list; ai != nullptr; ai = ai->ai_next) {
    if (ai->ai_family == AF_INET && ai->ai_addrlen >= sizeof(sockaddr_in)) {
      const auto *in = reinterpret_cast<const sockaddr_in *>(ai->ai_addr);
      sockaddr_storage storage{};
      std::memcpy(&storage, in, sizeof(sockaddr_in));
      if (auto addr = IPv4Address::from_sockaddr(
              storage, static_cast<socklen_t>(sizeof(sockaddr_in))
          );
          addr) {
        out.emplace_back(*addr);
      }
    } else if (
        ai->ai_family == AF_INET6 && ai->ai_addrlen >= sizeof(sockaddr_in6)
    ) {
      const auto *in6 = reinterpret_cast<const sockaddr_in6 *>(ai->ai_addr);
      sockaddr_storage storage{};
      std::memcpy(&storage, in6, sizeof(sockaddr_in6));
      if (auto addr = IPv6Address::from_sockaddr(
              storage, static_cast<socklen_t>(sizeof(sockaddr_in6))
          );
          addr) {
        out.emplace_back(*addr);
      }
    }
  }
  freeaddrinfo(list);

  if (out.empty()) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::NotFound, "No addresses found for {}", host
        )
    );
  }
  return out;
}

} // namespace net
