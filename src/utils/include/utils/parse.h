#pragma once

#include <charconv>
#include <concepts>
#include <optional>
#include <string_view>

namespace utils {

template <std::integral T>
std::optional<T> parse_number(std::string_view string) {
  if (string.empty()) {
    return std::nullopt;
  }
  T value = 0;
  const auto [ptr, ec] = std::from_chars(string.begin(), string.end(), value);
  if (ec != std::errc{} || ptr != string.end()) {
    return std::nullopt;
  }
  return value;
}

} // namespace utils
