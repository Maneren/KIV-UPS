#pragma once

#include <chrono>
#include <cstddef>
#include <net/socket.h>
#include <optional>
#include <span>
#include <string>
#include <utility>

namespace net {

class TcpStream {
  Socket sock;

public:
  explicit TcpStream(Socket &&sock);

  ~TcpStream();

  TcpStream(const TcpStream &) = delete;
  TcpStream(TcpStream &&other) noexcept;
  TcpStream &operator=(const TcpStream &) = delete;
  TcpStream &operator=(TcpStream &&other) noexcept;

  static error::result<TcpStream> connect(const SocketAddr &addr);
  static error::result<TcpStream>
  connect_timeout(const SocketAddr &addr, std::chrono::microseconds timeout);

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

  [[nodiscard]] error::result<TcpStream> try_clone() const;

  [[nodiscard]] error::result<SocketAddr> local_addr() const;
  [[nodiscard]] error::result<SocketAddr> peer_addr() const;
  [[nodiscard]] error::result<void> shutdown(Shutdown how) const;
  [[nodiscard]] error::result<void> set_nonblocking(bool nonblocking) const;
  [[nodiscard]] error::result<std::optional<error::IoError>> take_error() const;
  [[nodiscard]] error::result<void> set_nodelay(bool nodelay) const;
  [[nodiscard]] error::result<bool> nodelay() const;
  [[nodiscard]] error::result<void> set_ttl(uint32_t ttl) const;
  [[nodiscard]] error::result<uint32_t> ttl() const;
  [[nodiscard]] error::result<void>
  set_read_timeout(std::optional<std::chrono::microseconds> timeout) const;
  [[nodiscard]] error::result<std::optional<std::chrono::microseconds>>
  read_timeout() const;
  [[nodiscard]] error::result<void>
  set_write_timeout(std::optional<std::chrono::microseconds> timeout) const;
  [[nodiscard]] error::result<std::optional<std::chrono::microseconds>>
  write_timeout() const;

  [[nodiscard]] const Socket &socket() const noexcept { return sock; }
};

} // namespace net
