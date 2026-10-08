#include <charconv>
#include <concepts>
#include <cstring>
#include <memory>
#include <net/socket_addr.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <string_view>
#include <utils/functional.h>

namespace net {

namespace {

template <std::unsigned_integral T>
error::result<T> parse_number(std::string_view string, std::string_view what) {
  if (string.empty()) {
    return tl::make_unexpected(
        error::SimpleMessage(error::ErrorKind::InvalidInput, "Missing {}", what)
    );
  }
  T value = 0;
  const auto [ptr, ec] = std::from_chars(string.begin(), string.end(), value);
  if (ec != std::errc{} || ptr != string.end()) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid {}: {}", what, string
        )
    );
  }
  return value;
}

} // namespace

// SocketAddrV4

sockaddr_in SocketAddrV4::to_sockaddr() const noexcept {
  sockaddr_in addr_in{};
  addr_in.sin_family = AF_INET;
  addr_in.sin_port = htons(port_);

  static_assert(sizeof(addr_in.sin_addr.s_addr) == Ipv4Addr::BYTES);
  std::memcpy(&addr_in.sin_addr.s_addr, ip_.octets().data(), Ipv4Addr::BYTES);

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

error::result<SocketAddrV4> SocketAddrV4::from_string(std::string_view str) {
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

  const auto ip_part = str.substr(0, colon_pos);
  const auto port_part = str.substr(colon_pos + 1);

  const auto ip = Ipv4Addr::from_string(ip_part);
  if (!ip) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid socket address: {}", str
        )
    );
  }

  const auto port = parse_number<uint16_t>(port_part, "port number");
  if (!port) {
    return tl::make_unexpected(std::move(port).error());
  }

  return SocketAddrV4{*ip, *port};
}

// SocketAddrV6

sockaddr_in6 SocketAddrV6::to_sockaddr() const noexcept {
  sockaddr_in6 addr_in6{};
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
            SIZE
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

error::result<SocketAddrV6> SocketAddrV6::from_string(std::string_view str) {
  if (!str.starts_with('[')) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid socket address: {}", str
        )
    );
  }

  const size_t bracket_pos = str.find(']', 1);
  if (bracket_pos == std::string::npos) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid socket address: {}", str
        )
    );
  }

  auto ip_part = str.substr(1, bracket_pos - 1);
  const auto rest = str.substr(bracket_pos + 1);
  if (!rest.starts_with(':')) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput,
            "Missing port in socket address: {}",
            str
        )
    );
  }
  const auto port_part = rest.substr(1);

  uint32_t scope_id = 0;
  const size_t percent_pos = ip_part.find('%');
  if (percent_pos != std::string::npos) {
    const auto scope_part = ip_part.substr(percent_pos + 1);
    ip_part = ip_part.substr(0, percent_pos);
    const auto parsed_scope = parse_number<uint32_t>(scope_part, "scope id");
    if (!parsed_scope) {
      return tl::make_unexpected(std::move(parsed_scope).error());
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

  const auto port = parse_number<uint16_t>(port_part, "port number");
  if (!port) {
    return tl::make_unexpected(std::move(port).error());
  }

  return SocketAddrV6{*ip, *port, 0, scope_id};
}

// SocketAddr

SocketAddr::SocketAddr(const IpAddr &ip, uint16_t port)
    : inner(ip.visit(
          [port](const Ipv4Addr &v4) -> Inner {
            return SocketAddrV4(v4, port);
          },
          [port](const Ipv6Addr &v6) -> Inner { return SocketAddrV6(v6, port); }
      )) {}

IpAddr SocketAddr::ip() const {
  return visit([](const auto &addr) { return IpAddr(addr.ip()); });
}

void SocketAddr::set_ip(const IpAddr &ip) { *this = SocketAddr(ip, port()); }

uint16_t SocketAddr::port() const {
  return visit([](const auto &addr) { return addr.port(); });
}

void SocketAddr::set_port(uint16_t port) {
  visit([port](auto &addr) { addr.set_port(port); });
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
  using result_type = std::tuple<sockaddr_union, socklen_t>;
  return visit(
      [](const SocketAddrV4 &v4) -> result_type {
        return {sockaddr_union{.ipv4 = v4.to_sockaddr()}, SocketAddrV4::SIZE};
      },
      [](const SocketAddrV6 &v6) -> result_type {
        return {sockaddr_union{.ipv6 = v6.to_sockaddr()}, SocketAddrV6::SIZE};
      }
  );
}

error::result<SocketAddr> SocketAddr::from_string(std::string_view str) {
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
  struct addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *list = nullptr;
  const int code = getaddrinfo(host.c_str(), nullptr, &hints, &list);
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

  const std::unique_ptr<struct addrinfo, decltype(&freeaddrinfo)> guard(
      list, &freeaddrinfo
  );

  std::vector<SocketAddr> out;
  for (const struct addrinfo *ai = list; ai != nullptr; ai = ai->ai_next) {
    const sockaddr_union sockaddr{.sa = *ai->ai_addr};

    if (auto addr = SocketAddr::from_sockaddr(sockaddr, ai->ai_addrlen); addr) {
      addr->set_port(port);
      out.emplace_back(*addr);
    }
  }

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
