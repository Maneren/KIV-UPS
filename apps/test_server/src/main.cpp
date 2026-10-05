#include <iostream>
#include <net/listener.h>
#include <net/tcp_iostream.h>
#include <threadpool/threadpool.h>
#include <utils/print.h>

#include <charconv>
#include <optional>
#include <random>
#include <string>
#include <string_view>

namespace {

// Lines are terminated by '\n'; tolerate a single trailing '\r'.
void strip_cr(std::string &line) {
  if (!line.empty() && line.back() == '\r') {
    line.pop_back();
  }
}

// Strictly parse the whole line as an integer, with no headers or whitespace.
std::optional<int> parse_number(const std::string_view line) {
  int value = 0;
  const auto [ptr, ec] = std::from_chars(line.begin(), line.end(), value);
  // If there's and error or anything left, it's not a valid number
  if (ec != std::errc{} || ptr != line.end()) {
    return std::nullopt;
  }
  return value;
}

void send_error(net::TcpIostream &io) { io << "ERROR\n" << std::flush; }

void handle_client(net::TcpStream stream, net::Address address) {
  std::println("Accepted connection from {}", address);

  net::TcpIostream io{stream};
  std::string line;

  if (!std::getline(io, line)) {
    std::println("Client {} disconnected before HELLO", address);
    return;
  }
  strip_cr(line);
  if (line != "HELLO") {
    std::println("Client {}: expected HELLO, got \"{}\"", address, line);
    send_error(io);
    return;
  }

  thread_local std::mt19937 rng{std::random_device{}()};
  const int number = std::uniform_int_distribution<int>(1, 100)(rng);
  io << "NUM:" << number << '\n' << std::flush;

  if (!std::getline(io, line)) {
    std::println("Client {} disconnected before answer", address);
    return;
  }
  strip_cr(line);
  const auto answer = parse_number(line);
  if (!answer) {
    std::println("Client {}: expected a number, got \"{}\"", address, line);
    send_error(io);
    return;
  }

  const bool correct = *answer == 2 * number;
  std::println(
      "Client {}: answered {}, expected {} -> {}",
      address,
      *answer,
      2 * number,
      correct ? "OK" : "WRONG"
  );
  io << (correct ? "OK\n" : "WRONG\n") << std::flush;
}

} // namespace

int main(const int argc, const char *const *argv) {
  const std::span args{argv, static_cast<size_t>(argc)};

  if (argc > 2) {
    std::println("Usage: {} [address]", args.front());
    return 1;
  }

  threadpool::Threadpool pool;

  try {
    const char *addr_str = args.size() > 1 ? args[1] : "0.0.0.0:8080";
    const auto address = net::IPv4Address::from_string(addr_str);

    if (!address) {
      std::println("Invalid address: {}", addr_str);
      return 1;
    }

    const auto listener = net::TcpListener::bind(*address);

    if (!listener) {
      std::println("Failed to bind to {}: {}", addr_str, listener.error());
      return 1;
    }

    std::println("Listening on {}", *address);

    for (auto connection : listener->incoming()) {
      if (!connection) {
        std::println("Failed to accept connection: {}", connection.error());
        break;
      }

      auto &[stream, client_address] = *connection;

      pool.spawn([stream = std::move(stream), client_address] mutable {
        handle_client(std::move(stream), client_address);
      });
    }
  } catch (const std::exception &e) {
    std::println(std::cerr, "Unexpected error: {}", e.what());
    return 1;
  }

  pool.join();

  return 0;
}
