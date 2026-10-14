#pragma once

#include <charconv>
#include <optional>
#include <string_view>

namespace utils {

template <typename T, typename C = char>
  requires std::is_arithmetic_v<T>
std::optional<T> parse_number(std::basic_string_view<C> string) {
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

template <typename T, typename C = char>
  requires std::is_arithmetic_v<T>
std::optional<T> parse_number(std::basic_string<C> string) {
  return parse_number<T>(std::basic_string_view<C>{string});
}

} // namespace utils
