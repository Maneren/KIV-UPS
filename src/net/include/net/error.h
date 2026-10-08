#pragma once

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>
#include <utils/match.h>
#include <variant>

namespace net::error {

struct Os {
  // Plain error payload, intentionally aggregate-like.
  // NOLINTNEXTLINE(*non-private-member-variables-in-classes)
  int code;

  explicit Os(int code = errno) noexcept : code{code} {}
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
  Uncategorized,
};

std::string_view to_string(ErrorKind kind);
ErrorKind from_errno(int code);

struct Simple {
  // Plain error payload, intentionally aggregate-like.
  // NOLINTNEXTLINE(*non-private-member-variables-in-classes)
  ErrorKind kind;
  // NOLINTNEXTLINE(*non-private-member-variables-in-classes)
  std::string_view msg;

  explicit Simple(ErrorKind kind, std::string_view msg = {}) noexcept
      : kind{kind}, msg{msg} {}
};

struct SimpleMessage {
  // Plain error payload, intentionally aggregate-like.
  // NOLINTNEXTLINE(*non-private-member-variables-in-classes)
  ErrorKind kind;
  // NOLINTNEXTLINE(*non-private-member-variables-in-classes)
  std::string msg;

  SimpleMessage(ErrorKind kind, std::string msg) noexcept;

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
  // Implicit converting ctors are intentional: tl::expected<T, IoError> relies
  // on unexpected<Os/Simple/SimpleMessage> converting to IoError.
  // NOLINTNEXTLINE(*explicit-constructor)
  IoError(Os os) : inner{os} {}
  // NOLINTNEXTLINE(*explicit-constructor)
  IoError(Simple simple) : inner{simple} {}
  // NOLINTNEXTLINE(*explicit-constructor)
  IoError(SimpleMessage simple) : inner{std::move(simple)} {}

  [[nodiscard]] ErrorKind kind() const;

  [[nodiscard]] std::optional<int> os_code() const;

  template <typename... Fs>
  constexpr decltype(auto) visit(Fs &&...fs) const noexcept(
      noexcept(match::match(inner, std::forward<Fs>(fs)...))
  ) {
    return match::match(inner, std::forward<Fs>(fs)...);
  }

  template <typename... Fs>
  constexpr decltype(auto) visit(Fs &&...fs) noexcept(
      noexcept(match::match(inner, std::forward<Fs>(fs)...))
  ) {
    return match::match(inner, std::forward<Fs>(fs)...);
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

IoError last_os_error();
} // namespace net::error

template <> struct std::formatter<net::error::IoError> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(const net::error::IoError &err, std::format_context &ctx) {
    auto format_simple =
        [&ctx](net::error::ErrorKind kind, std::string_view msg) {
          if (msg.empty()) {
            return std::format_to(ctx.out(), "{}", net::error::to_string(kind));
          }
          return std::format_to(
              ctx.out(), "{}: {}", net::error::to_string(kind), msg
          );
        };
    return err.visit(
        [&ctx](const net::error::Os &os) {
          return std::format_to(
              ctx.out(), "{} - {}", os.code, std::strerror(os.code)
          );
        },
        [&](const net::error::Simple &simple) {
          return format_simple(simple.kind, simple.msg);
        },
        [&](const net::error::SimpleMessage &simple) {
          return format_simple(simple.kind, simple.msg);
        }
    );
  }
};
