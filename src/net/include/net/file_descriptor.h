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

  FileDescriptor(FileDescriptor &&other) noexcept;
  FileDescriptor &operator=(FileDescriptor &&other) noexcept;

  void close() noexcept;

  [[nodiscard]] constexpr bool valid() const noexcept { return fd != -1; }

  /// Release ownership without closing. Returns the raw fd, or -1.
  [[nodiscard]] int release() noexcept;

  [[nodiscard]]
  constexpr int raw() const noexcept {
    return this->fd;
  }
};

} // namespace net
