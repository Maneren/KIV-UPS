#include <algorithm>
#include <cli/cli.hpp>
#include <iostream>
#include <net/listener.h>
#include <net/tcp_iostream.h>
#include <utils/print.h>

constexpr auto ECHO_PORT = 2001;
constexpr auto REVERSE_PORT = 2002;
constexpr auto CALCULATE_PORT = 2003;

namespace {

int test_calculate(
    const std::string &address, int argc, const char *const *argv
) {
  const auto args = cli::Parser().option("o", "operation").parse(argc, argv);
  if (!args) {
    std::println("Failed to parse arguments: {}", args.error().what());
    return 1;
  }

  const auto operation = args->get_value("operation");
  if (!operation) {
    std::println("Missing operation");
    return 1;
  }

  if (args->positional().size() != 2) {
    std::println(
        std::cerr,
        "Expected 2 positional arguments, got {}",
        args->positional().size()
    );
    return 1;
  }

  const auto a = std::stol(args->positional()[0]);
  const auto b = std::stol(args->positional()[1]);

  std::println("Connecting to {}:{}", address, CALCULATE_PORT);
  auto stream = net::TcpStream::connect_host(address, CALCULATE_PORT);
  if (!stream) {
    std::println("Failed to connect to server: {}", stream.error());
    return 1;
  }

  std::println("Connected to {}", *stream->peer_addr());

  net::TcpIostream iostream{*stream};

  std::string received;
  constexpr auto HEADER_LINES = 4;
  for (const auto _ : std::views::iota(0, HEADER_LINES)) {
    std::getline(iostream, received);
    std::println("Received: {}", received);
  }

  std::println("Sending: {}|{}|{}", *operation, a, b);
  iostream << *operation << '|' << a << '|' << b << '\n' << std::flush;

  std::getline(iostream, received);
  std::println("Received: {}", received);

  return 0;
}

void test_echo(const std::string &address) {
  std::println("Connecting to {}:{}", address, ECHO_PORT);
  auto stream = net::TcpStream::connect_host(address, ECHO_PORT);
  if (!stream) {
    std::println("Failed to connect to server: {}", stream.error());
    return;
  }

  std::println("Connected to {}", *stream->peer_addr());

  net::TcpIostream iostream{*stream};

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

void test_reverse(const std::string &address) {
  std::println("Connecting to {}:{}", address, REVERSE_PORT);
  auto stream = net::TcpStream::connect_host(address, REVERSE_PORT);
  if (!stream) {
    std::println("Failed to connect to server: {}", stream.error());
    return;
  }

  std::println("Connected to {}", *stream->peer_addr());

  net::TcpIostream iostream{*stream};

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

int main(const int argc, const char *const *argv) {
  try {
    constexpr auto address = "kiv-ubl.kiv.zcu.cz";

    test_echo(address);
    test_reverse(address);
    if (auto res = test_calculate(address, argc, argv); res != 0) {
      return res;
    }
  } catch (const std::exception &e) {
    std::cerr << "Unexpected error: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
