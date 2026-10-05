#pragma once

#include <net/error.h>

namespace net {

class FileDescriptor {
  int fd;

public:
  FileDescriptor();
  explicit FileDescriptor(int fd);
  ~FileDescriptor();

  FileDescriptor(const FileDescriptor &) = delete;
  FileDescriptor &operator=(const FileDescriptor &) = delete;

  [[nodiscard]]
  error::result<FileDescriptor> duplicate() const;

  FileDescriptor(FileDescriptor &&other) noexcept : fd(other.fd) {
    other.fd = -1;
  }
  FileDescriptor &operator=(FileDescriptor &&other) noexcept;

  void close() noexcept;

  [[nodiscard]] constexpr bool valid() const { return fd != -1; }

  /// Release ownership without closing. Returns the raw fd, or -1.
  int release() noexcept {
    const int raw = fd;
    fd = -1;
    return raw;
  }

  [[nodiscard]]
  constexpr int raw() const {
    return this->fd;
  }
};

} // namespace net
