#pragma once

#include "utils/match.h"
#include <cerrno>
#include <cstring>
#include <format>
#include <optional>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>
#include <variant>

namespace net::error {

struct Os {
  int code;

  Os(int code = errno) : code{code} {}
};

enum class ErrorKind : std::uint8_t {
  NotFound,
  PermissionDenied,
  ConnectionRefused,
  ConnectionReset,
  HostUnreachable,
  NetworkUnreachable,
  ConnectionAborted,
  NotConnected,
  AddrInUse,
  AddrNotAvailable,
  NetworkDown,
  BrokenPipe,
  AlreadyExists,
  WouldBlock,
  NotADirectory,
  IsADirectory,
  DirectoryNotEmpty,
  ReadOnlyFilesystem,
  FilesystemLoop,
  StaleNetworkFileHandle,
  InvalidInput,
  InvalidData,
  TimedOut,
  WriteZero,
  StorageFull,
  NotSeekable,
  QuotaExceeded,
  FileTooLarge,
  ResourceBusy,
  ExecutableFileBusy,
  Deadlock,
  CrossesDevices,
  TooManyLinks,
  InvalidFilename,
  ArgumentListTooLong,
  Interrupted,
  Unsupported,
  UnexpectedEof,
  OutOfMemory,
  InProgress,
  Other,
  Uncategorized
};

std::string_view to_string(ErrorKind kind);

struct Simple {
  ErrorKind kind;
  std::string_view msg;

  Simple(ErrorKind kind, std::string_view msg = {}) : kind{kind}, msg{msg} {}
};

struct SimpleMessage {
  ErrorKind kind;
  std::string msg;

  SimpleMessage(ErrorKind kind, auto &msg) : kind{kind}, msg{msg} {}

  template <typename... Args>
  SimpleMessage(
      ErrorKind kind, const std::format_string<Args...> &msg, Args &&...args
  )
      : kind{kind}, msg{std::format(msg, std::forward<Args>(args)...)} {}
};

struct IoError {
private:
  using Variant = std::variant<Os, Simple, SimpleMessage>;
  Variant inner;

public:
  IoError(Variant data) : inner{std::move(data)} {}
  IoError(Variant &&data) : inner{std::move(data)} {}

  IoError(const Os &os) : inner{os} {}
  IoError(const Simple &simple) : inner{simple} {}
  IoError(const SimpleMessage &simple) : inner{simple} {}
  IoError(SimpleMessage &&simple) : inner{std::move(simple)} {}

  [[nodiscard]] std::optional<int> os_code() const {
    return match::match(
        inner,
        [](const Os &os) -> std::optional<int> { return os.code; },
        [](const auto &) -> std::optional<int> { return std::nullopt; }
    );
  }

  [[nodiscard]] const Variant &data() const { return inner; }
  [[nodiscard]] Variant &data() { return inner; }
};

template <typename T> using result = tl::expected<T, IoError>;

template <typename T> result<T> from_os(T code) {
  if (code == -1) {
    return tl::make_unexpected(Os{});
  }

  return code;
}

inline IoError last_os_error() { return {Os{errno}}; }

} // namespace net::error

template <> struct std::formatter<net::error::IoError> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(const auto &kind, std::format_context &ctx) {
    return match::match(
        kind.data(),
        [&ctx](const net::error::Os &os) {
          return std::format_to(
              ctx.out(), "{} - {}", os.code, std::strerror(os.code)
          );
        },
        [&ctx](const net::error::Simple &simple) {
          return std::format_to(
              ctx.out(), "{}", net::error::to_string(simple.kind)
          );
        },
        [&ctx](const net::error::SimpleMessage &simple) {
          return std::format_to(ctx.out(), "{}", simple.msg);
        }
    );
  }
};
