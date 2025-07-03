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
  return duplicate_fd(fd).map(functional::BindConstructor<FileDescriptor>());
}

error::result<int> FileDescriptor::duplicate_fd(int source_fd) {
  constexpr auto cmd = F_DUPFD_CLOEXEC;

  // There is no other way to do this
  // NOLINTNEXTLINE(*cppcoreguidelines-pro-type-vararg)
  const auto fd = ::fcntl(source_fd, cmd, 3);

  if (fd < 0) {
    return tl::make_unexpected(error::Os{errno});
  }

  return fd;
}

} // namespace net
