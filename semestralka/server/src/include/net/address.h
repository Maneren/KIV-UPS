#pragma once

#include <array>
#include <cstring>
#include <netinet/in.h>
#include <string>

namespace net {

struct Address {
  Address() {}
  virtual ~Address() {}
  virtual int family() const = 0;
  virtual sockaddr_in to_sockaddr() const = 0;

  virtual std::string to_string() const = 0;
};

struct IPv4Address : public Address {
  std::array<uint8_t, 4> octets;
  uint16_t port;

  constexpr static size_t BYTES = 4;

  IPv4Address() {}
  IPv4Address(std::array<uint8_t, BYTES> octets, uint16_t port)
      : octets(octets), port(port) {}

  ~IPv4Address() override = default;

  int family() const override { return AF_INET; }

  sockaddr_in to_sockaddr() const override;

  static IPv4Address
  from_sockaddr(const sockaddr_storage &storage, socklen_t len);

  std::string to_string() const override;
};

} // namespace net
