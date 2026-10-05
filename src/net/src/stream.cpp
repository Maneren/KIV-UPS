#include <cstring>
#include <net/stream.h>
#include <span>

namespace net {

TcpStream::TcpStream(Socket &&sock) : sock(std::move(sock)) {};

namespace {

error::result<TcpStream>
connect_each(const std::vector<SocketAddr> &addrs, auto connect_one) {
  error::IoError last_error(
      error::Simple{error::ErrorKind::NotFound, "no addresses to connect to"}
  );
  bool attempted = false;
  for (const auto &addr : addrs) {
    auto stream = connect_one(addr);
    if (stream) {
      return stream;
    }
    last_error = stream.error();
    attempted = true;
  }
  if (!attempted) {
    return tl::make_unexpected(
        error::SimpleMessage(
            error::ErrorKind::InvalidInput, "No addresses to connect to"
        )
    );
  }
  return tl::make_unexpected(last_error);
}

} // namespace

error::result<TcpStream> TcpStream::connect(const SocketAddr &addr) {
  return Socket::create(addr, SOCK_STREAM)
      .and_then([&addr](auto &&sock) {
        return sock.connect(addr).map([&] {
          return std::forward<Socket>(sock);
        });
      })
      .map(functional::Constructor<TcpStream>());
}
error::result<TcpStream> TcpStream::connect_timeout(
    const SocketAddr &addr, std::chrono::microseconds timeout
) {
  return Socket::create(addr, SOCK_STREAM)
      .and_then([&addr, timeout](auto &&sock) {
        return sock.connect_timeout(addr, timeout).map([&] {
          return std::forward<Socket>(sock);
        });
      })
      .map(functional::Constructor<TcpStream>());
}

error::result<TcpStream>
TcpStream::connect_host(const std::string &host, uint16_t port) {
  return SocketAddr::resolve(host, port).and_then([](const auto &addrs) {
    return connect_each(addrs, [](const SocketAddr &addr) {
      return TcpStream::connect(addr);
    });
  });
}

error::result<TcpStream> TcpStream::connect_timeout_host(
    const std::string &host, uint16_t port, std::chrono::microseconds timeout
) {
  return SocketAddr::resolve(host, port).and_then([timeout](const auto &addrs) {
    return connect_each(addrs, [timeout](const SocketAddr &addr) {
      return TcpStream::connect_timeout(addr, timeout);
    });
  });
}

error::result<void> TcpStream::read_exact(std::span<std::byte> buf) const {
  size_t done = 0;
  while (done < buf.size()) {
    const auto chunk = buf.subspan(done);
    const auto ret = read(chunk);
    if (!ret) {
      return tl::make_unexpected(ret.error());
    }
    if (*ret == 0) {
      return tl::make_unexpected(
          error::Simple{
              error::ErrorKind::UnexpectedEof, "eof while reading exact bytes"
          }
      );
    }
    done += static_cast<size_t>(*ret);
  }
  return {};
}

error::result<void> TcpStream::write_all(std::span<const std::byte> buf) const {
  size_t done = 0;
  while (done < buf.size()) {
    const auto chunk = buf.subspan(done);
    const auto ret = write(chunk);
    if (!ret) {
      return tl::make_unexpected(ret.error());
    }
    if (*ret == 0) {
      return tl::make_unexpected(
          error::Simple{
              error::ErrorKind::WriteZero, "failed to write all bytes"
          }
      );
    }
    done += static_cast<size_t>(*ret);
  }
  return {};
}

TcpStream::~TcpStream() = default;

TcpStream::TcpStream(TcpStream &&other) noexcept
    : sock(std::move(other.sock)) {}

TcpStream &TcpStream::operator=(TcpStream &&other) noexcept {
  this->sock = std::move(other.sock);
  return *this;
}

} // namespace net
