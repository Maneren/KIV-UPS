#include <fcntl.h>
#include <net/error.h>
#include <net/file_descriptor.h>
#include <unistd.h>
#include <utility>

namespace net {

FileDescriptor::FileDescriptor() : fd(INVALID_FD) {}
FileDescriptor::FileDescriptor(int fd) : fd(fd) {}
FileDescriptor::~FileDescriptor() { close(); }

FileDescriptor::FileDescriptor(FileDescriptor &&other) noexcept
    : fd(other.release()) {}

int FileDescriptor::release() noexcept { return std::exchange(fd, INVALID_FD); }

FileDescriptor &FileDescriptor::operator=(FileDescriptor &&other) noexcept {
  if (this != &other) {
    close();
    fd = other.release();
  }
  return *this;
}

void FileDescriptor::close() noexcept {
  if (fd != INVALID_FD) {
    ::close(fd);
    fd = INVALID_FD;
  }
}

error::result<FileDescriptor> FileDescriptor::duplicate() const {
  if (fd < 0) {
    return tl::make_unexpected(
        error::Simple{error::ErrorKind::InvalidInput, "Invalid file descriptor"}
    );
  }

  // There is no other way to do this
  // NOLINTNEXTLINE(*cppcoreguidelines-pro-type-vararg)
  return error::from_os(::fcntl(fd, F_DUPFD_CLOEXEC, 0)).map([](int new_fd) {
    return FileDescriptor(new_fd);
  });
}

} // namespace net
