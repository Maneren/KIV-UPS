#include <algorithm>
#include <iostream>
#include <net/listener.h>
#include <net/tcp_iostream.h>
#include <utils/print.h>

namespace {

void test_echo(net::IpAddr address) {
  const net::SocketAddr sockaddr{address, 2001};

  auto stream = net::TcpStream::connect(sockaddr).value();

  std::println("Connected to {}", sockaddr);

  net::TcpIostream iostream{stream};

  const auto *const line = "Hello World";
  std::println("Sending: {}", line);
  iostream << line << '\n' << std::flush;

  std::string received;
  std::getline(iostream, received);
  std::println("Received: {}", received);

  if (received != line) {
    std::println(
        std::cerr, R"(Echo response was "{}" instead of "{}")", received, line
    );
  }
}

void test_reverse(net::IpAddr address) {
  const net::SocketAddr sockaddr{address, 2002};

  auto stream = net::TcpStream::connect(sockaddr).value();

  std::println("Connected to {}", sockaddr);

  net::TcpIostream iostream{stream};

  std::string line;
  std::getline(iostream, line);
  std::println("Received: {}", line);

  std::ranges::reverse(line);
  std::println("Sending: {}", line);
  iostream << line << '\n' << std::flush;

  std::getline(iostream, line);
  std::println("Received: {}", line);
}

} // namespace

int main() {
  threadpool::Threadpool pool;

  try {
    const auto server = net::Ipv4Addr::from_string("147.228.67.67").value();

    test_echo(server);
    test_reverse(server);
  } catch (const std::exception &e) {
    std::cerr << "Unexpected error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
