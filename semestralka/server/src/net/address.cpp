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
  addr.port = addr_in->sin_port;

  static_assert(sizeof(addr_in->sin_addr.s_addr) == BYTES);
  std::memcpy(addr.octets.data(), &addr_in->sin_addr.s_addr, BYTES);

  return addr;
}

std::string IPv4Address::to_string() const {
  return std::to_string(octets[0]) + "." + std::to_string(octets[1]) + "." +
         std::to_string(octets[2]) + "." + std::to_string(octets[3]) + ":" +
         std::to_string(port);
}

} // namespace net
