#pragma once

#include <unistd.h>

namespace net {

struct FileDescriptor {
  int fd;

  FileDescriptor() : fd(-1) {}
  FileDescriptor(int fd) : fd(fd) {}
  ~FileDescriptor() { close(fd); }

  inline FileDescriptor(const FileDescriptor &other) { duplicate_fd(other.fd); }
  inline FileDescriptor &operator=(const FileDescriptor &other) {
    duplicate_fd(other.fd);
    return *this;
  }

  inline FileDescriptor(FileDescriptor &&other) : fd(other.fd) {
    other.fd = -1;
  }
  inline FileDescriptor &operator=(FileDescriptor &&other) {
    this->fd = other.fd;
    other.fd = -1;
    return *this;
  }

private:
  void duplicate_fd(const int source_fd);
};

} // namespace net
