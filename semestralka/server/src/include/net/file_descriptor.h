#pragma once

#include "net/error.h"
#include <unistd.h>

namespace net {

struct FileDescriptor {
  int fd;

  FileDescriptor();
  FileDescriptor(int fd);
  ~FileDescriptor();

  FileDescriptor(const FileDescriptor &other) : fd(-1) {
    duplicate_fd(other.fd);
  }
  FileDescriptor &operator=(const FileDescriptor &other) {
    duplicate_fd(other.fd);
    return *this;
  }

  FileDescriptor(FileDescriptor &&other) noexcept : fd(other.fd) {
    other.fd = -1;
  }
  FileDescriptor &operator=(FileDescriptor &&other) noexcept {
    this->fd = other.fd;
    other.fd = -1;
    return *this;
  }

private:
  [[nodiscard]]
  error::result<void> duplicate_fd(int source_fd);
};

} // namespace net
