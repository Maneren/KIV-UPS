#include <iostream>
#include <net/listener.h>
#include <ostream>
#include <sstream>
#include <threadpool/threadpool.h>
#include <utils/print.h>
#include <vector>

int main() {
  threadpool::Threadpool pool;

  try {
    const auto address = net::IPv4Address::from_string("0.0.0.0:8080").value();
    // const auto address = net::IPv6Address::from_string("[::]:8080");

    const auto listener = net::TcpListener::bind(address).value();

    std::println("Listening on {}", address);

    for (auto &&connection : listener.incoming()) {
      if (!connection) {
        std::println("Failed to accept connection: {}", connection.error());
        break;
      }

      auto &[client_stream, client_address] = connection.value();

      pool.spawn([client_stream = std::move(client_stream), client_address] {
        std::println(
            "Accepted connection from {} on socket {}",
            client_address,
            client_stream.socket().raw_fd()
        );

        std::vector<std::byte> buffer(128);

        while (true) {
          const auto read_result = client_stream.read(std::span(buffer));

          if (!read_result) {
            std::println("Failed to read from client: {}", read_result.error());
            break;
          }

          const auto bytes_read = read_result.value();

          if (bytes_read == 0) {
            std::println("Connection closed from {}", client_address);
            break;
          }

          const auto received_bytes = std::span(buffer.data(), bytes_read);

          std::stringstream ss;
          for (const auto &byte : received_bytes) {
            std::print(ss, "{:02x}", static_cast<int>(byte));
          }

          std::println(
              "Received {} bytes from address {}: {} (\"{}\")",
              bytes_read,
              client_address,
              ss.str(),
              std::string(received_bytes.begin(), received_bytes.end())
          );
        }
      });
    }
  } catch (const std::exception &e) {
    std::println(std::cerr, "Unexpected error: {}", e.what());
    return 1;
  }

  pool.join();

  return 0;
}
