#include <iostream>
#include <net/exception.h>
#include <net/listener.h>
#include <ostream>
#include <sstream>
#include <utils/print.h>
#include <vector>

int main() {
  try {
    const auto address = net::IPv4Address::from_string("0.0.0.0:8080");
    // const auto address = net::IPv6Address::from_string("[::]:8080");

    net::TcpListener listener(address);

    std::println("Listening on {}", address);

    while (true) {
      const auto [client_stream, client_address] = listener.accept();

      std::println("Accepted connection from {}", client_address);

      std::vector<std::byte> buffer(128);
      const auto bytes_read = client_stream.read(std::span(buffer));
      buffer.resize(bytes_read);

      std::stringstream ss;
      for (const auto &byte : buffer)
        ss << std::hex << static_cast<int>(byte);

      std::println("Received {} bytes: {}", bytes_read, ss.str());
    }
  } catch (const net::io_exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
