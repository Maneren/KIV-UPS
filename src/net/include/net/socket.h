#pragma once

#include <chrono>
#include <net/error.h>
#include <net/file_descriptor.h>
#include <net/socket_addr.h>
#include <optional>
#include <type_traits>
#include <utils/functional.h>
#include <utils/print.h>

namespace net {

enum class Shutdown : uint_fast8_t {
  Read,
  Write,
  Both,
};

class Socket {
  FileDescriptor fd;

public:
  Socket(FileDescriptor &&fd) : fd(std::move(fd)) {}

  static error::result<Socket> create(int family, int type);
  static error::result<Socket> create(const SocketAddr &addr, int type) {
    return create(addr.family(), type);
  };

  Socket(const Socket &) = delete;
  Socket &operator=(const Socket &) = delete;

  Socket(Socket &&other) noexcept : fd(std::move(other.fd)) {}
  Socket &operator=(Socket &&other) noexcept {
    this->fd = std::move(other.fd);
    return *this;
  }

  ~Socket() = default;

  [[nodiscard]] const FileDescriptor &file_descriptor() const { return fd; }
  [[nodiscard]] constexpr int raw_fd() const { return fd.raw(); };

  template <typename T>
  error::result<void> setopts(int level, int optname, const T &optval) const {
    static_assert(
        std::is_trivially_copyable_v<T>, "sockopt value must be trivial"
    );
    const auto code = setsockopt(
        raw_fd(), level, optname, &optval, static_cast<socklen_t>(sizeof(T))
    );

    return error::from_os(code).map(functional::drop);
  };

  template <typename T> error::result<T> getopts(int level, int optname) const {
    static_assert(
        std::is_trivially_copyable_v<T>, "sockopt value must be trivial"
    );
    T optval{};
    auto len = static_cast<socklen_t>(sizeof(T));

    return error::from_os(getsockopt(raw_fd(), level, optname, &optval, &len))
        .map([&](auto) { return optval; });
  };

  [[nodiscard]] error::result<void> bind_to(const SocketAddr &addr) const;

  [[nodiscard]]
  error::result<Socket>
  accept(sockaddr &storage, socklen_t &len, int flags = 0) const;

  [[nodiscard]] error::result<void> connect(const SocketAddr &addr) const;
  [[nodiscard]] error::result<void> connect_timeout(
      const SocketAddr &addr, std::chrono::microseconds timeout
  ) const;

  [[nodiscard]] error::result<std::optional<error::IoError>> take_error() const;

  [[nodiscard]] error::result<void> set_nonblocking(bool blocking) const;

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

  [[nodiscard]] error::result<Socket> duplicate() const {
    return fd.duplicate().map(functional::Constructor<Socket>());
  }

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
