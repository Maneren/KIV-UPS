#pragma once

#include <cstddef>
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

  // Non-owning pointer into user-owned TcpStream; streambuf is neither
  // copyable nor movable (get/put pointers would dangle).
  TcpStreambuf(const TcpStreambuf &) = delete;
  TcpStreambuf &operator=(const TcpStreambuf &) = delete;
  TcpStreambuf(TcpStreambuf &&) = delete;
  TcpStreambuf &operator=(TcpStreambuf &&) = delete;

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

  // Tied to the embedded streambuf; neither copyable nor movable.
  TcpIostream(const TcpIostream &) = delete;
  TcpIostream &operator=(const TcpIostream &) = delete;
  TcpIostream(TcpIostream &&) = delete;
  TcpIostream &operator=(TcpIostream &&) = delete;

  // Flush the output buffer
  void flush_output();

  // Get the underlying TcpStream
  TcpStream &tcp_stream() { return *streambuf_.stream_; }
  const TcpStream &tcp_stream() const { return *streambuf_.stream_; }
};

} // namespace net
