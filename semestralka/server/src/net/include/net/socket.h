#pragma once

#include "net/error.h"
#include "utils/functional.h"
#include <chrono>
#include <net/address.h>
#include <net/file_descriptor.h>
#include <utils/print.h>

namespace net {

class Socket {
  FileDescriptor fd;

public:
  Socket(FileDescriptor &&fd) : fd(std::move(fd)) {}

  static error::result<Socket> create(int family, int type);
  static error::result<Socket> create(const Address &addr, int type) {
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
  error::result<void> setopts(int level, int optname, const T &optval) {
    const auto code = setsockopt(
        raw_fd(), level, optname, &optval, static_cast<socklen_t>(sizeof(T))
    );

    return error::from_os(code).map(functional::drop);
  };

  template <typename T> error::result<T> getopts(int level, int optname) const {
    T optval;
    auto len = static_cast<socklen_t>(sizeof(T));

    return error::from_os(getsockopt(raw_fd(), level, optname, &optval, &len))
        .map([&](auto) { return optval; });
  };

  [[nodiscard]] error::result<void> bind_to(const Address &addr) const;

  [[nodiscard]]
  error::result<Socket>
  accept(sockaddr &storage, socklen_t &len, int flags = 0) const;

  [[nodiscard]] error::result<void> connect(const Address &addr) const;
  [[nodiscard]] error::result<void>
  connect_timeout(const Address &addr, std::chrono::microseconds timeout) const;

  [[nodiscard]] error::result<std::optional<error::IoError>>
  error_state() const;

  [[nodiscard]] error::result<void> set_nonblocking(bool blocking) const;

  ssize_t read(void *buf, size_t len) const;
  ssize_t write(const void *buf, size_t len) const;

  ssize_t recv(void *buf, size_t len, int flags) const;
  ssize_t send(const void *buf, size_t len, int flags) const;

  [[nodiscard]] error::result<Socket> duplicate() const {
    return fd.duplicate().map(functional::Constructor<Socket>());
  }
};

} // namespace net
