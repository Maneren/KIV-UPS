#include <net/tcp_iostream.h>

namespace net {

TcpStreambuf::TcpStreambuf(TcpStream *stream)
    : stream_(stream), input_buffer_(DEFAULT_BUFFER_SIZE),
      output_buffer_(DEFAULT_BUFFER_SIZE) {

  // Set up output buffer
  char *output_begin = output_buffer_.data();
  char *output_end = output_begin + output_buffer_.size() - 1;
  setp(output_begin, output_end);

  // Set up input buffer (initially empty)
  char *input_begin = input_buffer_.data();
  setg(input_begin, input_begin, input_begin);
}

TcpStreambuf::~TcpStreambuf() { sync(); }

std::streambuf::int_type TcpStreambuf::underflow() {
  if (gptr() < egptr()) {
    return traits_type::to_int_type(*gptr());
  }

  // Read more data from the stream
  const std::span<std::byte> buf(
      reinterpret_cast<std::byte *>(input_buffer_.data()), input_buffer_.size()
  );

  const auto result = stream_->read(buf);
  if (!result) {
    return traits_type::eof();
  }

  ssize_t bytes_read = result.value();
  if (bytes_read <= 0) {
    return traits_type::eof();
  }

  // Set up the get area
  char *buffer_begin = input_buffer_.data();
  setg(buffer_begin, buffer_begin, buffer_begin + bytes_read);

  return traits_type::to_int_type(*gptr());
}

std::streambuf::int_type TcpStreambuf::overflow(int_type ch) {
  if (ch != traits_type::eof()) {
    *pptr() = traits_type::to_char_type(ch);
    pbump(1);
  }

  if (pptr() == pbase()) {
    return traits_type::not_eof(ch);
  }

  if (!flush_output()) {
    if (this->pptr() > this->epptr()) {
      this->pbump(-1);
    }
    return traits_type::eof();
  }

  return traits_type::not_eof(ch);
}

int TcpStreambuf::sync() { return flush_output() ? 0 : -1; }

bool TcpStreambuf::flush_output() {
  const ptrdiff_t bytes_to_write = pptr() - pbase();
  if (bytes_to_write == 0) {
    return true;
  }

  const std::span<const std::byte> buf(
      reinterpret_cast<const std::byte *>(pbase()),
      static_cast<size_t>(bytes_to_write)
  );

  size_t total_written = 0;
  while (total_written < static_cast<size_t>(bytes_to_write)) {
    const auto remaining_buf = buf.subspan(total_written);
    const auto result = stream_->write(remaining_buf);
    if (!result) {
      return false;
    }

    ssize_t bytes_written = result.value();
    if (bytes_written == 0) {
      return false;
    }

    total_written += static_cast<size_t>(bytes_written);
  }

  // Reset the put area
  char *output_begin = output_buffer_.data();
  char *output_end = output_begin + output_buffer_.size() - 1;
  setp(output_begin, output_end);

  return true;
}

TcpIostream::TcpIostream(TcpStream &stream)
    : std::iostream(&streambuf_), streambuf_(&stream) {}

TcpIostream::TcpIostream(TcpIostream &&other) noexcept
    : std::iostream(&streambuf_), streambuf_(std::move(other.streambuf_)) {

  rdbuf(&streambuf_);
}

TcpIostream &TcpIostream::operator=(TcpIostream &&other) noexcept {
  if (this != &other) {
    streambuf_ = std::move(other.streambuf_);
    rdbuf(&streambuf_);
  }
  return *this;
}

void TcpIostream::flush_output() { streambuf_.flush_output(); }

} // namespace net
