#pragma once

#include <format>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace utils {

// Base formatter for types that do not accept a format spec.
struct formatter_base {
  constexpr auto parse(std::format_parse_context &ctx)
      -> std::format_parse_context::const_iterator {
    return ctx.begin();
  }
};

} // namespace utils

template <typename T, typename A>
  requires std::formattable<T, char>
struct std::formatter<std::vector<T, A>> : utils::formatter_base {
  static auto format(auto &obj, auto &ctx) {
    std::format_to(ctx.out(), "[");
    for (const auto &item : obj) {
      if (&item != &obj.front()) {
        std::format_to(ctx.out(), ", ");
      }
      std::format_to(ctx.out(), "{}", item);
    }
    return std::format_to(ctx.out(), "]");
  }
};

template <typename T, typename H, typename P, typename A>
  requires std::formattable<T, char>
struct std::formatter<std::unordered_set<T, H, P, A>> : utils::formatter_base {
  static auto format(auto &obj, auto &ctx) {
    std::format_to(ctx.out(), "{{");
    bool first = true;
    for (const auto &item : obj) {
      if (!first) {
        std::format_to(ctx.out(), ", ");
      }
      std::format_to(ctx.out(), "{}", item);
      first = false;
    }
    return std::format_to(ctx.out(), "}}");
  }
};

template <typename T, typename U, typename H, typename P, typename A>
  requires std::formattable<T, char> && std::formattable<U, char>
struct std::formatter<std::unordered_map<T, U, H, P, A>>
    : utils::formatter_base {
  static auto format(auto &obj, auto &ctx) {
    std::format_to(ctx.out(), "{{");
    bool first = true;
    for (const auto &[key, value] : obj) {
      if (!first) {
        std::format_to(ctx.out(), ", ");
      }
      std::format_to(ctx.out(), "{}: {}", key, value);
      first = false;
    }
    return std::format_to(ctx.out(), "}}");
  }
};

template <typename T>
  requires std::formattable<T, char>
struct std::formatter<std::optional<T>> : utils::formatter_base {
  static auto format(auto &obj, auto &ctx) {
    if (obj.has_value()) {
      return std::format_to(ctx.out(), "{}", obj.value());
    }
    return std::format_to(ctx.out(), "nullopt");
  }
};

template <typename T, typename U>
  requires std::formattable<T, char> && std::formattable<U, char>
struct std::formatter<std::pair<T, U>> : utils::formatter_base {
  static auto format(auto &obj, auto &ctx) {
    return std::format_to(ctx.out(), "({}, {})", obj.first, obj.second);
  }
};
