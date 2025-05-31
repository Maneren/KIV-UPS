#include "net/socket.h"
#include "net/address.h"
#include "net/exception.h"
#include <sys/socket.h>
#include <utils/print.h>

namespace net {

Socket::Socket(int family, int type) {
  int fd = socket(family, type | SOCK_CLOEXEC, 0);

  if (fd < 0) {
    throw io_exception("failed to create socket");
  }

  this->fd = fd;
}

void Socket::bind_to(const Address &addr) {
  const auto [sockaddr_union, len] = addr.to_sockaddr();

  const auto sockaddr =
      reinterpret_cast<const struct sockaddr *>(&sockaddr_union);

  if (bind(fd.fd, sockaddr, len) < 0) {
    throw io_exception("failed to bind socket");
  }

  std::println("Socket bound to {} with fd {}", addr, fd.fd);
}

Socket Socket::accept(sockaddr &storage, socklen_t &len) const {
  const auto fd = accept4(this->fd.fd, &storage, &len, SOCK_CLOEXEC);

  if (fd < 0) {
    throw io_exception("failed to accept connection");
  }

  return Socket(FileDescriptor{fd});
}

int Socket::read(void *buf, const size_t len) const {
  return recv(fd.fd, buf, len, 0);
}

int Socket::write(const void *buf, const size_t len) const {
  return send(fd.fd, buf, len, MSG_NOSIGNAL);
}

} // namespace net
