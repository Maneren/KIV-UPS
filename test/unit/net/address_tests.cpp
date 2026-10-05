#include <cstring>
#include <format>
#include <gtest/gtest.h>
#include <net/ip_addr.h>
#include <net/socket_addr.h>

class AddressTest : public ::testing::Test {

protected:
  void SetUp() override {};

  void TearDown() override {};
};

namespace net {

const Ipv4Addr ip4(127, 0, 0, 1);
const SocketAddrV4 addr4(ip4, 80);

TEST_F(AddressTest, IPv4Str) {
  ASSERT_EQ(std::format("{}", ip4), "127.0.0.1");
  ASSERT_EQ(std::format("{}", addr4), "127.0.0.1:80");
  ASSERT_EQ(std::format("{}", SocketAddr(addr4)), "127.0.0.1:80");
}

TEST_F(AddressTest, IPv4Equals) {
  ASSERT_EQ(ip4, ip4);
  ASSERT_NE(ip4, Ipv4Addr(127, 0, 0, 2));
  // Ports live on the socket address, not on the IP.
  ASSERT_EQ(SocketAddrV4(ip4, 80).ip(), SocketAddrV4(ip4, 81).ip());
  ASSERT_NE(SocketAddrV4(ip4, 80), SocketAddrV4(ip4, 81));
}

TEST_F(AddressTest, IPv4FromString) {
  ASSERT_EQ(Ipv4Addr::from_string("127.0.0.1"), ip4);
  // A port suffix belongs to SocketAddrV4.
  ASSERT_FALSE(Ipv4Addr::from_string("127.0.0.1:80").has_value());
}

TEST_F(AddressTest, SocketAddrV4Sockaddr) {
  const auto sockaddr = addr4.to_sockaddr();
  ASSERT_EQ(sockaddr.sin_family, AF_INET);
  ASSERT_EQ(sockaddr.sin_port, htons(80));
  ASSERT_TRUE(sockaddr.sin_addr.s_addr == htonl(0x7F000001));
}

TEST_F(AddressTest, SocketAddrV4FromSockaddr) {
  const auto sockaddr = addr4.to_sockaddr();
  const sockaddr_union addr4_storage = {.ipv4 = sockaddr};
  const auto addr =
      SocketAddrV4::from_sockaddr(addr4_storage, sizeof(sockaddr));
  ASSERT_TRUE(addr.has_value());
  ASSERT_EQ(addr.value(), addr4);
}

TEST_F(AddressTest, SocketAddrV4FromSockaddrWrongFamily) {
  auto sockaddr = addr4.to_sockaddr();
  sockaddr.sin_family = AF_INET6;

  const sockaddr_union addr4_storage = {.ipv4 = sockaddr};
  const auto addr =
      SocketAddrV4::from_sockaddr(addr4_storage, sizeof(sockaddr));

  ASSERT_FALSE(addr.has_value());
}

TEST_F(AddressTest, SocketAddrV4FromSockaddrWrongLen) {
  auto sockaddr = addr4.to_sockaddr();
  const sockaddr_union addr4_storage = {.ipv4 = sockaddr};

  const auto addr =
      SocketAddrV4::from_sockaddr(addr4_storage, sizeof(sockaddr) - 1);

  ASSERT_FALSE(addr.has_value());
}

const Ipv6Addr ip6({1, 2, 3, 4, 5, 6, 7, 8, 8, 7, 6, 5, 4, 3, 2, 1});
const SocketAddrV6 addr6(ip6, 80);

TEST_F(AddressTest, IPv6Str) {
  ASSERT_EQ(
      std::format("{}", SocketAddr(addr6)),
      "[102:304:506:708:807:605:403:201]:80"
  );
}

TEST_F(AddressTest, IPv6FromString) {
  ASSERT_EQ(Ipv6Addr::from_string("102:304:506:708:807:605:403:201"), ip6);
  // Bracketed + port form belongs to SocketAddrV6.
  ASSERT_FALSE(Ipv6Addr::from_string("[::1]:80").has_value());
  ASSERT_EQ(
      SocketAddrV6::from_string("[102:304:506:708:807:605:403:201]:80"), addr6
  );
}

TEST_F(AddressTest, IPv6Equals) {
  ASSERT_EQ(ip6, ip6);
  ASSERT_NE(ip6, Ipv6Addr(std::array<uint8_t, 16>{1, 2, 3, 4, 5, 6, 7, 9}));
  ASSERT_EQ(SocketAddrV6(ip6, 80).ip(), SocketAddrV6(ip6, 81).ip());
  ASSERT_NE(SocketAddrV6(ip6, 80), SocketAddrV6(ip6, 81));
}

TEST_F(AddressTest, SocketAddrV6Sockaddr) {
  const auto sockaddr = addr6.to_sockaddr();
  ASSERT_EQ(sockaddr.sin6_family, AF_INET6);
  ASSERT_EQ(sockaddr.sin6_port, htons(80));

  ASSERT_EQ(sizeof(sockaddr.sin6_addr.s6_addr), Ipv6Addr::BYTES);
  ASSERT_EQ(sockaddr.sin6_scope_id, addr6.scope_id());
  ASSERT_EQ(sockaddr.sin6_flowinfo, htonl(addr6.flowinfo()));

  ASSERT_TRUE(
      std::memcmp(
          sockaddr.sin6_addr.s6_addr, ip6.to_octets().data(), Ipv6Addr::BYTES
      ) == 0
  );
}

TEST_F(AddressTest, SocketAddrV4FromStringRejectsBadPorts) {
  ASSERT_FALSE(SocketAddrV4::from_string("127.0.0.1:99999").has_value());
  ASSERT_FALSE(SocketAddrV4::from_string("127.0.0.1:-1").has_value());
  ASSERT_FALSE(SocketAddrV4::from_string("127.0.0.1:80abc").has_value());
  ASSERT_FALSE(SocketAddrV4::from_string("127.0.0.1:").has_value());
  ASSERT_FALSE(SocketAddrV4::from_string("127.0.0.1").has_value());
  ASSERT_TRUE(Ipv4Addr::from_string("127.0.0.1").has_value());
}

TEST_F(AddressTest, SocketAddrV6FromStringRequiresBracketsAndPort) {
  ASSERT_TRUE(Ipv6Addr::from_string("::1").has_value());
  ASSERT_FALSE(Ipv6Addr::from_string("[::1]").has_value());
  ASSERT_TRUE(SocketAddrV6::from_string("[::1]:80").has_value());
  ASSERT_FALSE(SocketAddrV6::from_string("::1").has_value());
  ASSERT_FALSE(SocketAddrV6::from_string("[::1]").has_value());
  ASSERT_FALSE(SocketAddrV6::from_string("[::1]:99999").has_value());
  ASSERT_FALSE(SocketAddrV6::from_string("[::1").has_value());
  ASSERT_FALSE(SocketAddrV6::from_string("[::1]extra").has_value());
}

TEST_F(AddressTest, SocketAddrV6FromSockaddr) {
  const auto sockaddr = addr6.to_sockaddr();
  const sockaddr_union storage = {.ipv6 = sockaddr};
  const auto addr =
      SocketAddr::from_sockaddr(storage, sizeof(sockaddr)).value();
  ASSERT_EQ(addr, addr6);
}

TEST_F(AddressTest, SocketAddrPortAccessors) {
  SocketAddr any = SocketAddrV4(Ipv4Addr::from_string("127.0.0.1").value(), 80);
  ASSERT_EQ(any.port(), 80);
  ASSERT_TRUE(any.is_ipv4());
  ASSERT_FALSE(any.is_ipv6());
  any.set_port(8080);
  ASSERT_EQ(any.port(), 8080);

  const SocketAddr addr(IpAddr(Ipv4Addr::localhost()), 1234);
  ASSERT_EQ(addr.port(), 1234);
  ASSERT_EQ(addr.ip(), IpAddr(Ipv4Addr::localhost()));
}

TEST_F(AddressTest, AddressHelpers) {
  ASSERT_TRUE(Ipv4Addr::localhost().is_loopback());
  ASSERT_TRUE(Ipv4Addr::unspecified().is_unspecified());
  ASSERT_TRUE(Ipv4Addr::broadcast().is_broadcast());
  ASSERT_TRUE(Ipv4Addr(224, 0, 0, 1).is_multicast());
  ASSERT_TRUE(Ipv4Addr(192, 168, 1, 1).is_private());
  ASSERT_TRUE(Ipv6Addr::localhost().is_loopback());
  ASSERT_TRUE(Ipv6Addr::unspecified().is_unspecified());

  const SocketAddr any = SocketAddr::from_string("127.0.0.1:80").value();
  ASSERT_TRUE(any.is_ipv4());
  ASSERT_FALSE(any.is_ipv6());

  const auto resolved = SocketAddr::resolve("127.0.0.1", 80);
  ASSERT_TRUE(resolved.has_value());
  ASSERT_FALSE(resolved->empty());
}

TEST_F(AddressTest, ErrorKindFromErrno) {
  ASSERT_EQ(
      net::error::from_errno(ECONNREFUSED),
      net::error::ErrorKind::ConnectionRefused
  );
  ASSERT_EQ(net::error::from_errno(ETIMEDOUT), net::error::ErrorKind::TimedOut);
  ASSERT_EQ(
      net::error::IoError(net::error::Os{EACCES}).kind(),
      net::error::ErrorKind::PermissionDenied
  );
  ASSERT_EQ(
      net::error::IoError(net::error::Simple{net::error::ErrorKind::NotFound})
          .kind(),
      net::error::ErrorKind::NotFound
  );
}

} // namespace net
