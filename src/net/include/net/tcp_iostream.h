#pragma once

#include <iostream>
#include <net/stream.h>
#include <streambuf>
#include <vector>

namespace net {

class TcpStreambuf : public std::streambuf {
  TcpStream *stream_;
  std::vector<char> input_buffer_;
  std::vector<char> output_buffer_;
  static constexpr size_t DEFAULT_BUFFER_SIZE = 8192;

public:
  explicit TcpStreambuf(TcpStream *stream);
  ~TcpStreambuf() override;

  // Disable copy
  TcpStreambuf(const TcpStreambuf &) = delete;
  TcpStreambuf &operator=(const TcpStreambuf &) = delete;

  // Allow move
  TcpStreambuf(TcpStreambuf &&) = default;
  TcpStreambuf &operator=(TcpStreambuf &&) = default;

protected:
  // Input (reading)
  int_type underflow() override;

  // Output (writing)
  int_type overflow(int_type ch = traits_type::eof()) override;
  int sync() override;

private:
  bool flush_output();

  friend class TcpIostream;
};

class TcpIostream : public std::iostream {
  TcpStreambuf streambuf_;

public:
  explicit TcpIostream(TcpStream &stream);
  ~TcpIostream() override = default;

  // Disable copy
  TcpIostream(const TcpIostream &) = delete;
  TcpIostream &operator=(const TcpIostream &) = delete;

  // Allow move
  TcpIostream(TcpIostream &&other) noexcept;
  TcpIostream &operator=(TcpIostream &&other) noexcept;

  // Flush the output buffer
  void flush_output();

  // Get the underlying TcpStream
  TcpStream &tcp_stream() { return *streambuf_.stream_; }
  const TcpStream &tcp_stream() const { return *streambuf_.stream_; }
};

} // namespace net
