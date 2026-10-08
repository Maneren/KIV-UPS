#pragma once

#include <net/error.h>

namespace net {

class FileDescriptor {
  int fd;
  constexpr static int INVALID_FD = -1;

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

  [[nodiscard]] bool valid() const noexcept { return fd != INVALID_FD; }

  /// Release ownership without closing. Returns the raw fd, or -1.
  [[nodiscard]] int release() noexcept;

  [[nodiscard]] int raw() const noexcept { return fd; }
};

} // namespace net
