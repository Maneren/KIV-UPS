#include <gtest/gtest.h>
#include <latch>
#include <net/listener.h>
#include <net/stream.h>
#include <optional>
#include <string_view>
#include <thread>

namespace net {

const static Address address =
    IPv4Address::from_string("127.0.0.1:8080").value();

constexpr std::string_view message = "Hello, world!";

class StreamTest : public ::testing::Test {
  std::optional<std::jthread> server_thread = std::nullopt;
  std::latch server_latch{1};

protected:
  void wait_for_server() { server_latch.wait(); }

  void SetUp() override {
    server_thread.emplace([&] {
      const auto listener = TcpListener::bind(address).value();

      server_latch.count_down();

      auto result = listener.accept();

      if (!result) {
        std::println("Failed to accept connection");
        ASSERT_TRUE(false);
      }

      auto &[client_stream, client_address] = result.value();

      std::println(
          "Accepted connection from {} on socket {}",
          client_address,
          client_stream.socket().raw_fd()
      );

      std::array<std::byte, message.size()> buffer{};
      std::memcpy(buffer.data(), message.data(), message.size());

      const auto write_result =
          client_stream.write(std::span<std::byte>(buffer));

      if (!write_result) {
        std::println("Failed to write to client: {}", write_result.error());
        ASSERT_TRUE(false);
      }
    });
  };

  void TearDown() override {};
};

TEST_F(StreamTest, ClientConnectsToServer) {
  wait_for_server();

  const auto result = TcpStream::connect(address);

  if (!result) {
    std::println("Failed to connect to server: {}", result.error());
    ASSERT_TRUE(false);
  }

  const auto &stream = result.value();

  std::array<std::byte, 128> buffer{};

  const auto read_result = stream.read(std::span<std::byte>(buffer));

  ASSERT_TRUE(read_result);

  const auto recvd_message = std::string_view(
      reinterpret_cast<const char *>(buffer.data()),
      static_cast<size_t>(read_result.value())
  );

  std::println("Received message: {}", recvd_message);

  ASSERT_EQ(recvd_message, message);
}

TEST(StreamEphemeralTest, LocalAddrAndExactTransfer) {
  const auto bind_addr = IPv4Address::localhost(0);
  const auto listener = TcpListener::bind(bind_addr).value();
  const auto server_addr = listener.local_addr().value();
  ASSERT_NE(server_addr.port(), 0);

  std::jthread server([&] {
    auto [stream, peer] = listener.accept().value();
    std::array<std::byte, 4> buf{};
    ASSERT_TRUE(stream.read_exact(buf).has_value());
    ASSERT_TRUE(stream.write_all(buf).has_value());
  });

  auto client = TcpStream::connect(server_addr).value();
  ASSERT_TRUE(client.peer_addr().has_value());
  ASSERT_TRUE(client.set_nodelay(true).has_value());
  ASSERT_TRUE(client.nodelay().value());

  const std::array<std::byte, 4> out{
      std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}
  };
  ASSERT_TRUE(client.write_all(out).has_value());
  std::array<std::byte, 4> back{};
  ASSERT_TRUE(client.read_exact(back).has_value());
  ASSERT_EQ(back, out);
}

} // namespace net
