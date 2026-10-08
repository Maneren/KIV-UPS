#include <cstring>
#include <net/stream.h>
#include <span>

namespace net {

TcpStream::TcpStream(Socket &&sock) : sock(std::move(sock)) {};

namespace {

template <typename F, std::ranges::input_range R>
  requires std::invocable<F, const SocketAddr &> &&
           std::convertible_to<
               std::invoke_result_t<F, const SocketAddr &>,
               error::result<TcpStream>>
error::result<TcpStream> connect_each(const R &addrs, F connect_one) {
  std::optional<error::IoError> last_error;

  for (const auto &addr : addrs) {
    auto stream = connect_one(addr);
    if (stream) {
      return stream;
    }
    last_error = std::move(stream).error();
  }

  if (last_error) {
    return tl::make_unexpected(std::move(*last_error));
  }

  return tl::make_unexpected(
      error::Simple{
          error::ErrorKind::InvalidInput, "could not connect to any address"
      }
  );
}

template <typename Span, typename Op>
error::result<void>
pump_all(Span buf, Op op, error::ErrorKind empty_kind, std::string_view msg) {
  size_t done = 0;
  while (done < buf.size()) {
    auto ret = op(buf.subspan(done));
    if (!ret) {
      return tl::make_unexpected(std::move(ret).error());
    }
    if (*ret == 0) {
      return tl::make_unexpected(error::Simple{empty_kind, msg});
    }
    done += static_cast<size_t>(*ret);
  }
  return {};
}

} // namespace

error::result<TcpStream> TcpStream::connect(const SocketAddr &addr) {
  return Socket::create(addr, SOCK_STREAM)
      .and_then([&](auto &&sock) {
        return sock.connect(addr).map([&] { return std::move(sock); });
      })
      .map(functional::Constructor<TcpStream>());
}

error::result<TcpStream> TcpStream::connect(std::span<const SocketAddr> addrs) {
  return connect_each(addrs, [](const auto &addr) {
    return TcpStream::connect(addr);
  });
}

error::result<TcpStream> TcpStream::connect_timeout(
    const SocketAddr &addr, std::chrono::microseconds timeout
) {
  return Socket::create(addr, SOCK_STREAM)
      .and_then([&addr, timeout](auto &&sock) {
        return sock.connect_timeout(addr, timeout).map([&] {
          return std::move(sock);
        });
      })
      .map(functional::Constructor<TcpStream>());
}

error::result<TcpStream> TcpStream::connect_timeout(
    std::span<const SocketAddr> addrs, std::chrono::microseconds timeout
) {
  return connect_each(addrs, [timeout](const auto &addr) {
    return TcpStream::connect_timeout(addr, timeout);
  });
}

error::result<TcpStream>
TcpStream::connect_host(const std::string &host, uint16_t port) {
  return SocketAddr::resolve(host, port).and_then([](const auto &addrs) {
    return TcpStream::connect(addrs);
  });
}

error::result<TcpStream> TcpStream::connect_timeout_host(
    const std::string &host, uint16_t port, std::chrono::microseconds timeout
) {
  return SocketAddr::resolve(host, port).and_then([timeout](const auto &addrs) {
    return TcpStream::connect_timeout(addrs, timeout);
  });
}

error::result<void> TcpStream::read_exact(std::span<std::byte> buf) const {
  return pump_all(
      buf,
      [this](auto chunk) { return read(chunk); },
      error::ErrorKind::UnexpectedEof,
      "eof while reading exact bytes"
  );
}

error::result<void> TcpStream::write_all(std::span<const std::byte> buf) const {
  return pump_all(
      buf,
      [this](auto chunk) { return write(chunk); },
      error::ErrorKind::WriteZero,
      "failed to write all bytes"
  );
}

error::result<TcpStream> TcpStream::try_clone() const {
  return sock.duplicate().map([](Socket s) { return TcpStream(std::move(s)); });
}

error::result<SocketAddr> TcpStream::local_addr() const {
  return sock.local_addr();
}

error::result<SocketAddr> TcpStream::peer_addr() const {
  return sock.peer_addr();
}

error::result<void> TcpStream::shutdown(Shutdown how) const {
  return sock.shutdown(how);
}

error::result<void> TcpStream::set_nonblocking(bool nonblocking) const {
  return sock.set_nonblocking(nonblocking);
}

error::result<std::optional<error::IoError>> TcpStream::take_error() const {
  return sock.take_error();
}

error::result<void> TcpStream::set_nodelay(bool nodelay) const {
  return sock.set_nodelay(nodelay);
}

error::result<bool> TcpStream::nodelay() const { return sock.nodelay(); }

error::result<void> TcpStream::set_ttl(uint32_t ttl) const {
  return sock.set_ttl(ttl);
}

error::result<uint32_t> TcpStream::ttl() const { return sock.ttl(); }

error::result<void> TcpStream::set_read_timeout(
    std::optional<std::chrono::microseconds> timeout
) const {
  return sock.set_read_timeout(timeout);
}

error::result<std::optional<std::chrono::microseconds>>
TcpStream::read_timeout() const {
  return sock.read_timeout();
}

error::result<void> TcpStream::set_write_timeout(
    std::optional<std::chrono::microseconds> timeout
) const {
  return sock.set_write_timeout(timeout);
}

error::result<std::optional<std::chrono::microseconds>>
TcpStream::write_timeout() const {
  return sock.write_timeout();
}

} // namespace net
