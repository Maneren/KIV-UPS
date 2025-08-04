#pragma once

#include <concepts>
#include <format>
#include <unordered_map>
#include <unordered_set>
#include <vector>

template <typename T>
  requires(std::semiregular<std::formatter<T>>)
struct std::formatter<std::vector<T>> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
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

template <typename T>
  requires(std::semiregular<std::formatter<T>>)
struct std::formatter<std::unordered_set<T>> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
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

template <typename T, typename U>
  requires(
      std::semiregular<std::formatter<T>> && std::semiregular<std::formatter<U>>
  )
struct std::formatter<std::unordered_map<T, U>> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
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
  requires(std::semiregular<std::formatter<T>>)
struct std::formatter<std::optional<T>> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    if (obj.has_value()) {
      return std::format_to(ctx.out(), "{}", obj.value());
    }
    return std::format_to(ctx.out(), "nullopt");
  }
};
