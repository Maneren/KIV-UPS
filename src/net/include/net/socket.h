#pragma once

#include <chrono>
#include <cstddef>
#include <net/error.h>
#include <net/file_descriptor.h>
#include <net/socket_addr.h>
#include <optional>
#include <sys/socket.h>
#include <type_traits>
#include <utils/functional.h>

namespace net {

enum class Shutdown : uint_fast8_t {
  Read,
  Write,
  Both,
};

class Socket {
  FileDescriptor fd;

public:
  explicit Socket(FileDescriptor &&fd);

  static error::result<Socket> create(int family, int type);
  static error::result<Socket> create(const SocketAddr &addr, int type);

  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;

  Socket(Socket &&other) noexcept;
  Socket &operator=(Socket &&other) noexcept;

  ~Socket() = default;

  [[nodiscard]] const FileDescriptor &file_descriptor() const noexcept {
    return fd;
  }
  [[nodiscard]] int raw_fd() const noexcept { return fd.raw(); };

  template <typename T>
    requires std::is_trivially_copyable_v<T>
  error::result<void> setopt(int level, int optname, const T &optval) const {
    const auto code = setsockopt(
        raw_fd(), level, optname, &optval, static_cast<socklen_t>(sizeof(T))
    );

    return error::from_os(code).map(functional::drop);
  };

  template <typename T>
    requires std::is_trivially_copyable_v<T>
  error::result<T> getopt(int level, int optname) const {
    T optval{};
    auto len = static_cast<socklen_t>(sizeof(T));

    return error::from_os(getsockopt(raw_fd(), level, optname, &optval, &len))
        .map([val = std::move(optval)](auto) mutable {
          return std::move(val);
        });
  };

  [[nodiscard]] error::result<void> bind_to(const SocketAddr &addr) const;

  [[nodiscard]]
  error::result<Socket>
  accept(sockaddr_union &sockaddr, socklen_t &len, int flags = 0) const;

  [[nodiscard]] error::result<void> connect(const SocketAddr &addr) const;
  [[nodiscard]] error::result<void> connect_timeout(
      const SocketAddr &addr, std::chrono::microseconds timeout
  ) const;

  [[nodiscard]] error::result<std::optional<error::IoError>> take_error() const;

  [[nodiscard]] error::result<void> set_nonblocking(bool nonblocking) const;

  // Single syscalls with EINTR retry. Errors come back as result.
  [[nodiscard]] error::result<ssize_t> read(void *buf, size_t len) const;
  [[nodiscard]] error::result<ssize_t> write(const void *buf, size_t len) const;

  [[nodiscard]] error::result<ssize_t>
  recv(void *buf, size_t len, int flags) const;
  [[nodiscard]] error::result<ssize_t>
  send(const void *buf, size_t len, int flags) const;

  [[nodiscard]] error::result<SocketAddr> local_addr() const;
  [[nodiscard]] error::result<SocketAddr> peer_addr() const;

  [[nodiscard]] error::result<void> shutdown(Shutdown how) const;

  [[nodiscard]] error::result<Socket> duplicate() const;

  [[nodiscard]] error::result<void> set_reuseaddr(bool reuse) const;
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
};

} // namespace net
