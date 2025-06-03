#include <iostream>
#include <net/exception.h>
#include <net/listener.h>
#include <ostream>
#include <sstream>
#include <threadpool/threadpool.h>
#include <utils/print.h>
#include <vector>

int main() {
  threadpool::Threadpool pool;

  try {
    const auto address = net::IPv4Address::from_string("0.0.0.0:8080");
    // const auto address = net::IPv6Address::from_string("[::]:8080");

    const net::TcpListener listener(address);

    std::println("Listening on {}", address);

    while (true) {
      auto [client_stream, client_address] = listener.accept();

      pool.spawn([client_stream = std::move(client_stream), client_address] {
        std::println(
            "Accepted connection from {} on socket {}",
            client_address,
            client_stream.socket().fd.fd
        );

        std::vector<std::byte> buffer(128);

        while (true) {
          const auto bytes_read = client_stream.recv(buffer);

          if (bytes_read == 0) {
            std::println("Connection closed from {}", client_address);
            break;
          }

          const auto received_bytes = std::span(buffer.data(), bytes_read);

          std::stringstream ss;
          for (const auto &byte : received_bytes)
            ss << std::format("{:02x}", static_cast<int>(byte));

          std::println(
              "Received {} bytes from address {}: {}",
              bytes_read,
              client_address,
              ss.str()
          );
        }
      });
    }
  } catch (const net::io_exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
    return 1;
  }

  pool.join();

  return 0;
}
