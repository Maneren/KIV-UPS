#include <iostream>
#include <net/exception.h>
#include <net/listener.h>
#include <ostream>

int main() {
  try {
    const auto address = net::IPv4Address({127, 0, 0, 1}, 8080);

    net::TcpListener listener(address);

    std::cout << "Listening on " << address.to_string() << std::endl;

    while (true) {
      const auto [client_socket, client_address] = listener.accept();

      std::cout << "Accepted connection from " << client_address->to_string()
                << std::endl;
    }
  } catch (const net::IoException &e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
