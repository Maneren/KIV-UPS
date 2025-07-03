#pragma once

#include "net/error.h"
#include <unistd.h>

namespace net {

class FileDescriptor {
  int fd;

public:
  FileDescriptor();
  FileDescriptor(int fd);
  ~FileDescriptor();

  FileDescriptor(const FileDescriptor &) = delete;
  FileDescriptor &operator=(const FileDescriptor &) = delete;

  [[nodiscard]]
  error::result<FileDescriptor> duplicate() const;

  FileDescriptor(FileDescriptor &&other) noexcept : fd(other.fd) {
    other.fd = -1;
  }
  FileDescriptor &operator=(FileDescriptor &&other) noexcept {
    this->fd = other.fd;
    other.fd = -1;
    return *this;
  }

  [[nodiscard]]
  constexpr int raw() const {
    return this->fd;
  }

private:
  [[nodiscard]]
  static error::result<int> duplicate_fd(int source_fd);
};

} // namespace net
