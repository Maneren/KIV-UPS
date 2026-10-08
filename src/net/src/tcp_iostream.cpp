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

TcpStreambuf::~TcpStreambuf() {
  // Destructor cannot propagate errors; best effort flush.
  (void)sync();
}

std::streambuf::int_type TcpStreambuf::underflow() {
  if (gptr() < egptr()) {
    return traits_type::to_int_type(*gptr());
  }

  // Read more data from the stream
  const std::span<char> buf(input_buffer_.data(), input_buffer_.size());

  const auto result = stream_->read(buf);
  if (!result) {
    return traits_type::eof();
  }

  const ssize_t bytes_read = result.value();
  if (bytes_read <= 0) {
    return traits_type::eof();
  }

  // Set up the get area
  const std::span<char> read(
      input_buffer_.data(), static_cast<size_t>(bytes_read)
  );
  setg(read.data(), read.data(), &*read.end());

  return traits_type::to_int_type(*gptr());
}

std::streambuf::int_type TcpStreambuf::overflow(int_type ch) {
  if (ch != traits_type::eof()) {
    if (pptr() == epptr()) {
      if (!flush_output()) {
        return traits_type::eof();
      }
    }
    *pptr() = traits_type::to_char_type(ch);
    pbump(1);
  }

  if (!flush_output()) {
    return traits_type::eof();
  }

  return traits_type::not_eof(ch);
}

int TcpStreambuf::sync() { return flush_output() ? 0 : -1; }

bool TcpStreambuf::flush_output() {
  const auto pending = pptr() - pbase();
  if (pending <= 0) {
    return true;
  }
  const auto bytes_to_write = static_cast<size_t>(pending);

  const std::span<const char> chars(pbase(), bytes_to_write);
  if (!stream_->write_all(std::as_bytes(chars)).has_value()) {
    return false;
  }

  // Reset the put area, keeping one slot reserved so overflow can
  // always store its character before flushing (matches the ctor).
  const std::span<char> buf(output_buffer_.data(), output_buffer_.size() - 1);
  setp(buf.data(), &*buf.end());

  return true;
}

TcpIostream::TcpIostream(TcpStream &stream)
    : std::iostream(&streambuf_), streambuf_(&stream) {}

void TcpIostream::flush_output() { streambuf_.flush_output(); }

} // namespace net
