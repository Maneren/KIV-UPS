#include "net/file_descriptor.h"
#include "net/exception.h"
#include <fcntl.h>

namespace net {

FileDescriptor::FileDescriptor() : fd(-1) {}
FileDescriptor::FileDescriptor(int fd) : fd(fd) {}
FileDescriptor::~FileDescriptor() {
  if (this->fd > 0) {
    close(fd);
    this->fd = -1;
  }
}

void FileDescriptor::duplicate_fd(int source_fd) {
  const auto cmd = F_DUPFD_CLOEXEC;
  const auto fd = fcntl(source_fd, cmd, 3);

  if (fd < 0) {
    throw io_exception("failed to duplicate file descriptor");
  }

  this->fd = fd;
}

} // namespace net
