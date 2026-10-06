#include <fcntl.h>
#include <net/error.h>
#include <net/file_descriptor.h>
#include <unistd.h>

namespace net {

FileDescriptor::FileDescriptor() : fd(-1) {}
FileDescriptor::FileDescriptor(int fd) : fd(fd) {}
FileDescriptor::~FileDescriptor() { close(); }

FileDescriptor::FileDescriptor(FileDescriptor &&other) noexcept : fd(other.fd) {
  other.fd = -1;
}

int FileDescriptor::release() noexcept {
  const int raw = fd;
  fd = -1;
  return raw;
}

FileDescriptor &FileDescriptor::operator=(FileDescriptor &&other) noexcept {
  if (this != &other) {
    close();
    this->fd = other.fd;
    other.fd = -1;
  }
  return *this;
}

void FileDescriptor::close() noexcept {
  if (this->fd != -1) {
    ::close(fd);
    this->fd = -1;
  }
}

error::result<FileDescriptor> FileDescriptor::duplicate() const {
  if (fd < 0) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "Invalid file descriptor"
        )
    );
  }

  constexpr auto cmd = F_DUPFD_CLOEXEC;

  // There is no other way to do this
  // NOLINTNEXTLINE(*cppcoreguidelines-pro-type-vararg)
  const auto new_fd = ::fcntl(fd, cmd, 0);

  return error::from_os(new_fd).map([](int raw) {
    return FileDescriptor(raw);
  });
}

} // namespace net
