#pragma once

#include <cstring>
#include <format>
#include <string>
#include <string_view>

namespace net {

class io_exception : public std::runtime_error {

public:
  io_exception(std::string_view msg) : std::runtime_error(std::string(msg)) {}
  io_exception(std::string &msg) : std::runtime_error(msg) {}

  io_exception(std::string_view msg, int errno_)
      : std::runtime_error(
            std::string(std::format("{}, caused by: {}", msg, strerror(errno_)))
        ) {}
  io_exception(std::string &msg, int errno_)
      : net::io_exception(std::string_view(msg), errno_) {}

  template <typename... Args>
  io_exception(std::format_string<Args...> msg, Args &&...args)
      : std::runtime_error(std::format(msg, std::forward<Args>(args)...)) {}
};

} // namespace net
