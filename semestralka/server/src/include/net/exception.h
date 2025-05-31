#pragma once

#include <format>
#include <string>
#include <string_view>

namespace net {

class io_exception : public std::runtime_error {

public:
  io_exception(std::string_view msg) : std::runtime_error(std::string(msg)) {}
  io_exception(std::string &msg) : std::runtime_error(msg) {}

  template <typename... Args>
  io_exception(std::format_string<Args...> msg, Args &&...args)
      : std::runtime_error(std::format(msg, std::forward<Args>(args)...)) {}
};

} // namespace net
