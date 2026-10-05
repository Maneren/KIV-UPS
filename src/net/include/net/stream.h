#pragma once

#include <chrono>
#include <cstddef>
#include <net/socket.h>
#include <optional>
#include <span>
#include <string>

namespace net {

class TcpStream {
  Socket sock;

public:
  TcpStream(Socket &&sock);

  ~TcpStream();

  TcpStream(const TcpStream &) = delete;
  TcpStream(TcpStream &&other) noexcept;
  TcpStream &operator=(const TcpStream &) = delete;
  TcpStream &operator=(TcpStream &&other) noexcept;

  static error::result<TcpStream> connect(const Address &addr);
  static error::result<TcpStream>
  connect_timeout(const Address &addr, std::chrono::microseconds timeout);

  static error::result<TcpStream>
  connect_host(const std::string &host, uint16_t port);
  static error::result<TcpStream> connect_timeout_host(
      const std::string &host, uint16_t port, std::chrono::microseconds timeout
  );

  template <typename T, size_t Extent>
  [[nodiscard]] error::result<ssize_t> read(std::span<T, Extent> buf) const {
    const auto byte_buf = std::as_writable_bytes(buf);
    return sock.read(byte_buf.data(), byte_buf.size());
  };
  template <typename T, size_t Extent>
  [[nodiscard]] error::result<ssize_t> write(std::span<T, Extent> buf) const {
    const auto byte_buf = std::as_bytes(buf);
    return sock.write(byte_buf.data(), byte_buf.size());
  };

  template <typename T, size_t Extent>
  [[nodiscard]] error::result<ssize_t>
  recv(std::span<T, Extent> buf, int flags = 0) const {
    const auto byte_buf = std::as_writable_bytes(buf);
    return sock.recv(byte_buf.data(), byte_buf.size(), flags);
  };
  template <typename T, size_t Extent>
  [[nodiscard]] error::result<ssize_t>
  send(std::span<T, Extent> buf, int flags = 0) const {
    const auto byte_buf = std::as_bytes(buf);
    return sock.send(byte_buf.data(), byte_buf.size(), flags);
  };

  // Peek at incoming data without consuming it (MSG_PEEK).
  template <typename T, size_t Extent>
  [[nodiscard]] error::result<ssize_t> peek(std::span<T, Extent> buf) const {
    const auto byte_buf = std::as_writable_bytes(buf);
    return sock.recv(byte_buf.data(), byte_buf.size(), MSG_PEEK);
  }

  [[nodiscard]] error::result<void> read_exact(std::span<std::byte> buf) const;
  [[nodiscard]] error::result<void>
  write_all(std::span<const std::byte> buf) const;

  [[nodiscard]] error::result<TcpStream> try_clone() const {
    return sock.duplicate().map([](Socket s) {
      return TcpStream(std::move(s));
    });
  }

  [[nodiscard]] error::result<Address> local_addr() const {
    return sock.local_addr();
  }
  [[nodiscard]] error::result<Address> peer_addr() const {
    return sock.peer_addr();
  }
  [[nodiscard]] error::result<void> shutdown(Shutdown how) const {
    return sock.shutdown(how);
  }
  [[nodiscard]] error::result<void> set_nonblocking(bool nonblocking) const {
    return sock.set_nonblocking(nonblocking);
  }
  [[nodiscard]] error::result<std::optional<error::IoError>>
  take_error() const {
    return sock.take_error();
  }
  [[nodiscard]] error::result<void> set_nodelay(bool nodelay) const {
    return sock.set_nodelay(nodelay);
  }
  [[nodiscard]] error::result<bool> nodelay() const { return sock.nodelay(); }
  [[nodiscard]] error::result<void> set_ttl(uint32_t ttl) const {
    return sock.set_ttl(ttl);
  }
  [[nodiscard]] error::result<uint32_t> ttl() const { return sock.ttl(); }
  [[nodiscard]] error::result<void>
  set_read_timeout(std::optional<std::chrono::microseconds> timeout) const {
    return sock.set_read_timeout(timeout);
  }
  [[nodiscard]] error::result<std::optional<std::chrono::microseconds>>
  read_timeout() const {
    return sock.read_timeout();
  }
  [[nodiscard]] error::result<void>
  set_write_timeout(std::optional<std::chrono::microseconds> timeout) const {
    return sock.set_write_timeout(timeout);
  }
  [[nodiscard]] error::result<std::optional<std::chrono::microseconds>>
  write_timeout() const {
    return sock.write_timeout();
  }

  [[nodiscard]] const Socket &socket() const { return sock; }
};

} // namespace net
