#pragma once

#include <format>
#include <string>
#include <string_view>

namespace net {

class IoException : public std::exception {
  std::string msg;

public:
  IoException(std::string_view msg) : msg(msg) {}

  template <typename... Args>
  IoException(std::format_string<Args...> msg, Args &&...args)
      : msg(std::format(msg, std::forward<Args>(args)...)) {}

  const char *what() const noexcept override { return msg.c_str(); }
};

} // namespace net
