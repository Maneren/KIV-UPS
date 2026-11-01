#pragma once

#include <charconv>
#include <optional>
#include <string_view>

namespace utils {

constexpr std::size_t DEFAULT_BASE = 10;

template <typename T, typename C = char>
  requires std::is_arithmetic_v<T>
std::optional<T>
parse_number(std::basic_string_view<C> string, std::size_t base) {
  if (string.empty()) {
    return std::nullopt;
  }
  T value = 0;
  const auto [ptr, ec] =
      std::from_chars(string.begin(), string.end(), value, base);
  if (ec != std::errc{} || ptr != string.end()) {
    return std::nullopt;
  }
  return value;
}

template <typename T, std::size_t BASE = DEFAULT_BASE, typename C = char>
  requires std::is_arithmetic_v<T>
std::optional<T> parse_number(std::basic_string_view<C> string) {
  if (string.empty()) {
    return std::nullopt;
  }
  T value = 0;
  const auto [ptr, ec] =
      std::from_chars(string.begin(), string.end(), value, BASE);
  if (ec != std::errc{} || ptr != string.end()) {
    return std::nullopt;
  }
  return value;
}

template <typename T, typename C = char>
  requires std::is_arithmetic_v<T>
std::optional<T> parse_number(const std::basic_string<C> &string) {
  return parse_number<T>(std::basic_string_view<C>{string});
}

} // namespace utils
