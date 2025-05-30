#include "net/socket.h"
#include "net/address.h"
#include "net/exception.h"
#include <print>
#include <sys/socket.h>

namespace net {

Socket::Socket(int family, int type) {
  int fd = socket(family, type | SOCK_CLOEXEC, 0);

  if (fd < 0) {
    throw IoException("failed to create socket");
  }

  this->fd = fd;
}

void Socket::bind_to(const Address &addr) {
  const auto sockaddr_in = addr.to_sockaddr();
  const auto sockaddr = reinterpret_cast<const struct sockaddr *>(&sockaddr_in);

  if (bind(fd.fd, sockaddr, sizeof(sockaddr_in)) < 0) {
    throw IoException("failed to bind socket");
  }

  std::println("Socket bound to {} with fd {}", addr.to_string(), fd.fd);
}

Socket Socket::accept(sockaddr *storage, socklen_t *len) {
  const auto fd = accept4(this->fd.fd, storage, len, SOCK_CLOEXEC);

  if (fd < 0) {
    throw IoException("failed to accept connection");
  }

  return Socket(FileDescriptor{fd});
}

} // namespace net
