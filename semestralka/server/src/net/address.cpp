#include "net/address.h"
#include "net/exception.h"
#include <netinet/in.h>

namespace net {

sockaddr_in net::IPv4Address::to_sockaddr() const {
  sockaddr_in addr_in;
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
    throw IoException(
        "Invalid address length: {} < {}", len, sizeof(sockaddr_in)
    );
  }

  const auto addr_in = reinterpret_cast<const sockaddr_in *>(&storage);

  IPv4Address addr;
  addr.port = ntohs(addr_in->sin_port);

  static_assert(sizeof(addr_in->sin_addr.s_addr) == BYTES);
  std::memcpy(addr.octets.data(), &addr_in->sin_addr.s_addr, BYTES);

  return addr;
}

sockaddr_in6 net::IPv6Address::to_sockaddr() const {
  sockaddr_in6 addr_in6;
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
    throw IoException(
        "Invalid address length: {} < {}", len, sizeof(sockaddr_in6)
    );
  }

  const auto addr_in6 = reinterpret_cast<const sockaddr_in6 *>(&storage);

  IPv6Address addr;
  addr.port = ntohs(addr_in6->sin6_port);
  addr.flowinfo = ntohl(addr_in6->sin6_flowinfo);
  addr.scopeid = ntohl(addr_in6->sin6_scope_id);

  static_assert(sizeof(addr_in6->sin6_addr.s6_addr) == BYTES);
  std::memcpy(addr.octets.data(), &addr_in6->sin6_addr.s6_addr, BYTES);

  return addr;
}

} // namespace net
