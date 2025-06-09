#include <net/address.h>
#include <net/exception.h>
#include <netinet/in.h>
#include <string>

namespace net {

sockaddr_in net::IPv4Address::to_sockaddr() const {
  sockaddr_in addr_in{};
  std::memset(&addr_in, 0, sizeof(addr_in));
  addr_in.sin_family = AF_INET;
  addr_in.sin_port = htons(port);

  static_assert(sizeof(addr_in.sin_addr.s_addr) == BYTES);
  std::memcpy(&addr_in.sin_addr.s_addr, octets.data(), BYTES);

  return addr_in;
}

IPv4Address
IPv4Address::from_sockaddr(const sockaddr_storage &storage, socklen_t len) {
  if (len < sizeof(sockaddr_in)) {
    throw io_exception(
        "Invalid address length: {} < {}", len, sizeof(sockaddr_in)
    );
  }

  const auto addr_in = reinterpret_cast<const sockaddr_in *>(&storage);

  if (addr_in->sin_family != FAMILY) {
    throw io_exception(
        "Invalid address family for IPv4: {}", addr_in->sin_family
    );
  }

  static_assert(sizeof(addr_in->sin_addr.s_addr) == BYTES);
  return IPv4Address{ntohl(addr_in->sin_addr.s_addr), ntohs(addr_in->sin_port)};
}

IPv4Address IPv4Address::from_string(const std::string &str) {
  if (str.empty()) {
    throw io_exception("Invalid IPv4 address: {}", str);
  }

  const size_t colon_pos = str.find(':');
  const std::string ip_part =
      (colon_pos == std::string::npos) ? str : str.substr(0, colon_pos);
  const std::string port_part =
      (colon_pos == std::string::npos) ? "" : str.substr(colon_pos + 1);

  struct in_addr addr{};
  if (inet_pton(AF_INET, ip_part.c_str(), &addr) != 1) {
    throw io_exception("Invalid IPv4 address: {}", str);
  }

  uint16_t port = 0;
  if (!port_part.empty()) {
    try {
      port = static_cast<uint16_t>(std::stoi(port_part));
    } catch (const std::exception &) {
      throw io_exception("Invalid port number: {}", port_part);
    }
  }

  return {ntohl(addr.s_addr), port};
}

sockaddr_in6 net::IPv6Address::to_sockaddr() const {
  sockaddr_in6 addr_in6{};
  std::memset(&addr_in6, 0, sizeof(addr_in6));
  addr_in6.sin6_family = AF_INET6;
  addr_in6.sin6_port = htons(port);
  addr_in6.sin6_flowinfo = htonl(flowinfo);
  addr_in6.sin6_scope_id = htonl(scopeid);

  static_assert(sizeof(addr_in6.sin6_addr.s6_addr) == BYTES);
  std::memcpy(&addr_in6.sin6_addr.s6_addr, octets.data(), BYTES);

  return addr_in6;
}

IPv6Address
IPv6Address::from_sockaddr(const sockaddr_storage &storage, socklen_t len) {
  if (len < sizeof(sockaddr_in6)) {
    throw io_exception(
        "Invalid address length: {} < {}", len, sizeof(sockaddr_in6)
    );
  }

  const auto addr_in6 = reinterpret_cast<const sockaddr_in6 *>(&storage);

  if (addr_in6->sin6_family != FAMILY) {
    throw io_exception(
        "Invalid address family for IPv6: {}", addr_in6->sin6_family
    );
  }

  static_assert(sizeof(addr_in6->sin6_addr.s6_addr) == BYTES);
  return {
      static_cast<const uint8_t *>(addr_in6->sin6_addr.s6_addr),
      ntohs(addr_in6->sin6_port),
      ntohl(addr_in6->sin6_flowinfo),
      ntohl(addr_in6->sin6_scope_id)
  };
}

IPv6Address IPv6Address::from_string(const std::string &str) {
  if (str.empty()) {
    throw io_exception("Invalid IPv6 address: {}", str);
  }

  size_t colon_pos = std::string::npos;

  if (str.starts_with("[")) {
    const size_t bracket_pos = str.find(']');

    if (bracket_pos == std::string::npos) {
      throw io_exception("Invalid IPv6 address: {}", str);
    }

    colon_pos = str.rfind(':');
  }

  const std::string ip_part =
      (colon_pos == std::string::npos) ? str : str.substr(1, colon_pos - 2);
  const std::string port_part =
      (colon_pos == std::string::npos) ? "" : str.substr(colon_pos + 1);

  struct in6_addr addr{};
  if (inet_pton(AF_INET6, ip_part.c_str(), &addr) != 1) {
    throw io_exception("Invalid IPv6 address: {}", str);
  }

  uint16_t port = 0;
  if (!port_part.empty()) {
    try {
      port = static_cast<uint16_t>(std::stoi(port_part));
    } catch (const std::exception &) {
      throw io_exception("Invalid port number: {}", port_part);
    }
  }

  return {static_cast<uint8_t *>(addr.s6_addr), port};
}

Address Address::from_sockaddr(sockaddr_storage &storage, size_t len) {
  switch (storage.ss_family) {
  case IPv4Address::FAMILY:
    return {IPv4Address::from_sockaddr(storage, len)};
  case IPv6Address::FAMILY:
    return {IPv6Address::from_sockaddr(storage, len)};
  default:
    throw io_exception("unrecognized socket family");
  }
}

std::tuple<Address::sockaddr_union, int> Address::to_sockaddr() const {
  return match::match(
      inner,
      [](const IPv4Address &ipv4) {
        return std::make_tuple(
            sockaddr_union{.ipv4 = ipv4.to_sockaddr()}, sizeof(sockaddr_in)
        );
      },
      [](const IPv6Address &ipv6) {
        return std::make_tuple(
            sockaddr_union{.ipv6 = ipv6.to_sockaddr()}, sizeof(sockaddr_in6)
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

} // namespace net
