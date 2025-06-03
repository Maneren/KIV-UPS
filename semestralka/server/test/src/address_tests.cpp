#include <cstring>
#include <format>
#include <gtest/gtest.h>
#include <net/address.h>
#include <net/exception.h>

namespace net {

class AddressTest : public ::testing::Test {

protected:
  virtual void SetUp() {};

  virtual void TearDown() {};
};

template <typename T, typename U>
constexpr bool
bytes_equals(T a, const std::array<U, sizeof(T) / sizeof(U)> &b) {
  for (size_t i = 0; i < sizeof(T); i++) {
    if (static_cast<U>(a >> (i * 8)) != b[i])
      return false;
  }
  return true;
}

const IPv4Address addr4({127, 0, 0, 1}, 80);

TEST_F(AddressTest, IPv4Str) {
  const auto addr_str = std::format("{}", addr4);
  ASSERT_EQ(addr_str, "127.0.0.1:80");
}

TEST_F(AddressTest, IPv4Equals) {
  ASSERT_EQ(addr4, addr4);
  const auto other_addr = IPv4Address({127, 0, 0, 2}, 80);
  ASSERT_NE(addr4, other_addr);
  const auto other_port = IPv4Address({127, 0, 0, 1}, 81);
  ASSERT_NE(addr4, other_port);
}

TEST_F(AddressTest, IPv4FromString) {
  const auto addr = IPv4Address::from_string("127.0.0.1:80");
  ASSERT_EQ(addr, addr4);
}

TEST_F(AddressTest, IPv4Sockaddr) {
  const auto sockaddr = addr4.to_sockaddr();
  ASSERT_EQ(sockaddr.sin_family, AF_INET);
  ASSERT_EQ(sockaddr.sin_port, htons(addr4.port));
  ASSERT_TRUE(
      std::memcmp(
          &sockaddr.sin_addr, addr4.octets.data(), IPv4Address::BYTES
      ) == 0
  );
}

TEST_F(AddressTest, IPv4FromSockaddr) {
  const auto sockaddr = addr4.to_sockaddr();
  const auto addr = IPv4Address::from_sockaddr(
      reinterpret_cast<const sockaddr_storage &>(sockaddr), sizeof(sockaddr)
  );
  ASSERT_EQ(addr, addr4);
}

TEST_F(AddressTest, IPv4FromSockaddrWrongFamily) {
  auto sockaddr = addr4.to_sockaddr();
  sockaddr.sin_family = AF_INET6;
  ASSERT_THROW(
      IPv4Address::from_sockaddr(
          reinterpret_cast<const sockaddr_storage &>(sockaddr), sizeof(sockaddr)
      ),
      io_exception
  );
}

TEST_F(AddressTest, IPv4FromSockaddrWrongLen) {
  auto sockaddr = addr4.to_sockaddr();
  ASSERT_THROW(
      IPv4Address::from_sockaddr(
          reinterpret_cast<const sockaddr_storage &>(sockaddr),
          sizeof(sockaddr) - 1
      ),
      io_exception
  );
}

const IPv6Address addr6({1, 2, 3, 4, 5, 6, 7, 8, 8, 7, 6, 5, 4, 3, 2, 1}, 80);

TEST_F(AddressTest, IPv6Str) {
  const auto addr_str = std::format("{}", addr6);
  ASSERT_EQ(addr_str, "[102:304:506:708:807:605:403:201]:80");
}

TEST_F(AddressTest, IPv6FromString) {
  const auto addr =
      IPv6Address::from_string("[102:304:506:708:807:605:403:201]:80");
  ASSERT_EQ(addr, addr6);
}

TEST_F(AddressTest, IPv6Equals) {
  ASSERT_EQ(addr6, addr6);
  const auto other_addr = IPv6Address({1, 2, 3, 4, 5, 6, 7, 9}, 80, 0, 0);
  ASSERT_NE(addr6, other_addr);
  const auto other_port = IPv6Address({1, 2, 3, 4, 5, 6, 7, 8}, 81, 0, 0);
  ASSERT_NE(addr6, other_port);
}

TEST_F(AddressTest, IPv6Sockaddr) {
  const auto sockaddr = addr6.to_sockaddr();
  ASSERT_EQ(sockaddr.sin6_family, AF_INET6);
  ASSERT_EQ(sockaddr.sin6_port, htons(addr6.port));

  ASSERT_EQ(sizeof(sockaddr.sin6_addr.s6_addr), IPv6Address::BYTES);
  ASSERT_EQ(sockaddr.sin6_scope_id, htonl(addr6.scopeid));
  ASSERT_EQ(sockaddr.sin6_flowinfo, htonl(addr6.flowinfo));

  ASSERT_TRUE(
      std::memcmp(
          sockaddr.sin6_addr.s6_addr, addr6.octets.data(), IPv6Address::BYTES
      ) == 0
  );
}

TEST_F(AddressTest, IPv6FromSockaddr) {
  const auto sockaddr = addr6.to_sockaddr();
  const auto addr = IPv6Address::from_sockaddr(
      reinterpret_cast<const sockaddr_storage &>(sockaddr), sizeof(sockaddr)
  );
  ASSERT_EQ(addr, addr6);
}

} // namespace net
