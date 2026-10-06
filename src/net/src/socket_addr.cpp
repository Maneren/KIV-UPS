#include <charconv>
#include <cstring>
#include <net/socket_addr.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <utils/functional.h>
#include <utils/match.h>
#include <variant>

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

error::result<uint32_t> parse_scope_id(const std::string_view scope_part) {
  if (scope_part.empty()) {
    return tl::make_unexpected(
        error::SimpleMessage(error::ErrorKind::InvalidInput, "Missing scope id")
    );
  }
  unsigned long value = 0;
  const auto [ptr, ec] =
      std::from_chars(scope_part.begin(), scope_part.end(), value);
  if (ec != std::errc{} || ptr != scope_part.end() || value > UINT32_MAX) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid scope id: {}", scope_part
        )
    );
  }
  return static_cast<uint32_t>(value);
}

} // namespace

// SocketAddrV4

sockaddr_in SocketAddrV4::to_sockaddr() const {
  sockaddr_in addr_in{};
  addr_in.sin_family = AF_INET;
  addr_in.sin_port = htons(port_);

  static_assert(sizeof(addr_in.sin_addr.s_addr) == Ipv4Addr::BYTES);
  std::memcpy(&addr_in.sin_addr.s_addr, ip_.octets.data(), Ipv4Addr::BYTES);

  return addr_in;
}

error::result<SocketAddrV4>
SocketAddrV4::from_sockaddr(const sockaddr_union &sockaddr, socklen_t len) {
  if (len < SIZE) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Invalid address length: {} < {}",
            len,
            SIZE
        )
    );
  }

  if (sockaddr.sa.sa_family != FAMILY) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Invalid address family for IPv4: {:x}",
            sockaddr.sa.sa_family
        )
    );
  }

  const auto &ipv4 = sockaddr.ipv4;

  static_assert(sizeof(ipv4.sin_addr.s_addr) == Ipv4Addr::BYTES);
  return SocketAddrV4{
      Ipv4Addr{ntohl(ipv4.sin_addr.s_addr)}, ntohs(ipv4.sin_port)
  };
}

error::result<SocketAddrV4> SocketAddrV4::from_string(const std::string &str) {
  if (str.empty()) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid socket address: {}", str
        )
    );
  }

  const size_t colon_pos = str.rfind(':');
  if (colon_pos == std::string::npos) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Missing port in socket address: {}",
            str
        )
    );
  }

  const std::string ip_part = str.substr(0, colon_pos);
  const std::string port_part = str.substr(colon_pos + 1);

  const auto ip = Ipv4Addr::from_string(ip_part);
  if (!ip) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid socket address: {}", str
        )
    );
  }

  const auto port = parse_port(port_part);
  if (!port) {
    return tl::make_unexpected(port.error());
  }

  return SocketAddrV4{*ip, *port};
}

// SocketAddrV6

sockaddr_in6 SocketAddrV6::to_sockaddr() const {
  sockaddr_in6 addr_in6{};
  std::memset(&addr_in6, 0, sizeof(addr_in6));
  addr_in6.sin6_family = AF_INET6;
  addr_in6.sin6_port = htons(port_);
  addr_in6.sin6_flowinfo = htonl(flowinfo_);
  addr_in6.sin6_scope_id = scope_id_;

  static_assert(sizeof(addr_in6.sin6_addr.s6_addr) == Ipv6Addr::BYTES);
  std::memcpy(
      &addr_in6.sin6_addr.s6_addr, ip_.to_octets().data(), Ipv6Addr::BYTES
  );

  return addr_in6;
}

error::result<SocketAddrV6>
SocketAddrV6::from_sockaddr(const sockaddr_union &sockaddr, socklen_t len) {
  if (len < SIZE) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Invalid IPv6 address length: {} < {}",
            len,
            sizeof(sockaddr_in6)
        )
    );
  }

  if (sockaddr.sa.sa_family != FAMILY) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Invalid address family for IPv6: {}",
            sockaddr.sa.sa_family
        )
    );
  }

  const auto &ipv6 = sockaddr.ipv6;

  static_assert(sizeof(ipv6.sin6_addr.s6_addr) == Ipv6Addr::BYTES);
  return SocketAddrV6{
      Ipv6Addr{static_cast<const uint8_t *>(ipv6.sin6_addr.s6_addr)},
      ntohs(ipv6.sin6_port),
      ntohl(ipv6.sin6_flowinfo),
      ipv6.sin6_scope_id
  };
}

error::result<SocketAddrV6> SocketAddrV6::from_string(const std::string &str) {
  if (str.empty() || !str.starts_with('[')) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid socket address: {}", str
        )
    );
  }

  const size_t bracket_pos = str.find(']');
  if (bracket_pos == std::string::npos) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid socket address: {}", str
        )
    );
  }

  std::string ip_part = str.substr(1, bracket_pos - 1);
  const std::string rest = str.substr(bracket_pos + 1);
  if (rest.empty() || !rest.starts_with(':')) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Missing port in socket address: {}",
            str
        )
    );
  }
  const std::string port_part = rest.substr(1);

  uint32_t scope_id = 0;
  const size_t percent_pos = ip_part.find('%');
  if (percent_pos != std::string::npos) {
    const std::string scope_part = ip_part.substr(percent_pos + 1);
    ip_part = ip_part.substr(0, percent_pos);
    const auto parsed_scope = parse_scope_id(scope_part);
    if (!parsed_scope) {
      return tl::make_unexpected(parsed_scope.error());
    }
    scope_id = *parsed_scope;
  }

  const auto ip = Ipv6Addr::from_string(ip_part);
  if (!ip) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid socket address: {}", str
        )
    );
  }

  const auto port = parse_port(port_part);
  if (!port) {
    return tl::make_unexpected(port.error());
  }

  return SocketAddrV6{*ip, *port, 0, scope_id};
}

// SocketAddr

SocketAddr::SocketAddr(const IpAddr &ip, uint16_t port)
    : inner(SocketAddrV4(Ipv4Addr(), port)) {
  match::match(
      ip.inner,
      [port, this](const Ipv4Addr &v4) { inner = SocketAddrV4(v4, port); },
      [port, this](const Ipv6Addr &v6) { inner = SocketAddrV6(v6, port); }
  );
}

IpAddr SocketAddr::ip() const {
  return match::match(
      inner,
      [](const SocketAddrV4 &v4) { return IpAddr(v4.ip()); },
      [](const SocketAddrV6 &v6) { return IpAddr(v6.ip()); }
  );
}

void SocketAddr::set_ip(const IpAddr &ip) {
  match::match(
      inner,
      [&ip](SocketAddrV4 &v4) {
        if (const auto *v = std::get_if<Ipv4Addr>(&ip.inner)) {
          v4.set_ip(*v);
        }
      },
      [&ip](SocketAddrV6 &v6) {
        if (const auto *v = std::get_if<Ipv6Addr>(&ip.inner)) {
          v6.set_ip(*v);
        }
      }
  );
}

uint16_t SocketAddr::port() const {
  return match::match(
      inner,
      [](const SocketAddrV4 &v4) { return v4.port(); },
      [](const SocketAddrV6 &v6) { return v6.port(); }
  );
}

void SocketAddr::set_port(uint16_t port) {
  match::match(
      inner,
      [port](SocketAddrV4 &v4) { v4.set_port(port); },
      [port](SocketAddrV6 &v6) { v6.set_port(port); }
  );
}

error::result<SocketAddr>
SocketAddr::from_sockaddr(const sockaddr_union &sockaddr, socklen_t len) {
  switch (sockaddr.sa.sa_family) {
  case SocketAddrV4::FAMILY:
    return SocketAddrV4::from_sockaddr(sockaddr, len)
        .map(functional::Constructor<SocketAddr>());
  case SocketAddrV6::FAMILY:
    return SocketAddrV6::from_sockaddr(sockaddr, len)
        .map(functional::Constructor<SocketAddr>());
  default:
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Unrecognized socket family: {}",
            sockaddr.sa.sa_family
        )
    );
  }
}

std::tuple<sockaddr_union, socklen_t> SocketAddr::to_sockaddr() const {
  return match::match(
      inner,
      [](const SocketAddrV4 &v4) {
        return std::make_tuple(
            sockaddr_union{.ipv4 = v4.to_sockaddr()},
            static_cast<socklen_t>(sizeof(sockaddr_in))
        );
      },
      [](const SocketAddrV6 &v6) {
        return std::make_tuple(
            sockaddr_union{.ipv6 = v6.to_sockaddr()},
            static_cast<socklen_t>(sizeof(sockaddr_in6))
        );
      }
  );
}

error::result<SocketAddr> SocketAddr::from_string(const std::string &str) {
  if (const auto v4 = SocketAddrV4::from_string(str); v4) {
    return SocketAddr(*v4);
  }
  if (const auto v6 = SocketAddrV6::from_string(str); v6) {
    return SocketAddr(*v6);
  }
  return tl::make_unexpected(
      error::SimpleMessage(
          error::ErrorKind::InvalidInput, "Invalid socket address: {}", str
      )
  );
}

error::result<std::vector<SocketAddr>>
SocketAddr::resolve(const std::string &host, uint16_t port) {
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

  std::vector<SocketAddr> out;
  for (const struct addrinfo *ai = list; ai != nullptr; ai = ai->ai_next) {

    const sockaddr_union sockaddr{.sa = *ai->ai_addr};

    if (auto addr = SocketAddr::from_sockaddr(sockaddr, ai->ai_addrlen); addr) {
      out.emplace_back(*addr);
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
