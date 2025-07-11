#include <net/error.h>
#include <cerrno>
#include <fcntl.h>
#include <net/file_descriptor.h>

namespace net {

FileDescriptor::FileDescriptor() : fd(-1) {}
FileDescriptor::FileDescriptor(int fd) : fd(fd) {}
FileDescriptor::~FileDescriptor() {
  if (this->fd > 0) {
    close(fd);
    this->fd = -1;
  }
}

error::result<FileDescriptor> FileDescriptor::duplicate() const {
  constexpr auto cmd = F_DUPFD_CLOEXEC;

  // There is no other way to do this
  // NOLINTNEXTLINE(*cppcoreguidelines-pro-type-vararg)
  const auto new_fd = ::fcntl(fd, cmd, 3);

  if (fd < 0) {
    return error::from_os(errno);
  }

  return {new_fd};
}

} // namespace net
