#include "net/file_descriptor.h"
#include "net/exception.h"
#include <fcntl.h>

namespace net {

void FileDescriptor::duplicate_fd(int source_fd) {
  const auto cmd = F_DUPFD_CLOEXEC;
  const auto fd = fcntl(source_fd, cmd, 3);

  if (fd < 0) {
    throw io_exception("failed to duplicate file descriptor");
  }

  this->fd = fd;
}

} // namespace net
