#pragma once

#include <Color.hpp>
#include <Matrix.hpp>
#include <Rectangle.hpp>
#include <Vector2.hpp>
#include <Vector3.hpp>
#include <Vector4.hpp>
#include <format>

#define RAYLIB_CPP_FORMATTER(TYPE, ...)                                        \
  template <> struct std::formatter<raylib::TYPE> {                            \
    [[nodiscard]] static constexpr auto                                        \
    parse(std::format_parse_context const &ctx) {                              \
      return ctx.begin();                                                      \
    }                                                                          \
    template <typename FormatContext>                                          \
    auto format(const TYPE &obj, FormatContext &ctx) const {                   \
      return std::format_to(ctx.out(), __VA_ARGS__);                           \
    }                                                                          \
  };

#define RAYLIB_C_FORMATTER(TYPE, ...)                                          \
  template <> struct std::formatter<TYPE> {                                    \
    [[nodiscard]] static constexpr auto                                        \
    parse(std::format_parse_context const &ctx) {                              \
      return ctx.begin();                                                      \
    }                                                                          \
    template <typename FormatContext>                                          \
    auto format(const raylib::TYPE &obj, FormatContext &ctx) const {           \
      return std::format_to(ctx.out(), __VA_ARGS__);                           \
    }                                                                          \
  }

// Helper macro to define formatters for both C and C++ versions
#define RAYLIB_DUAL_FORMATTER(TYPE, ...)                                       \
  RAYLIB_CPP_FORMATTER(TYPE, __VA_ARGS__)                                      \
  RAYLIB_C_FORMATTER(TYPE, __VA_ARGS__)

RAYLIB_DUAL_FORMATTER(Vector2, "({}, {})", obj.x, obj.y);

RAYLIB_DUAL_FORMATTER(Vector3, "({}, {}, {})", obj.x, obj.y, obj.z);

RAYLIB_DUAL_FORMATTER(Vector4, "({}, {}, {}, {})", obj.x, obj.y, obj.z, obj.w);

RAYLIB_DUAL_FORMATTER(Color, "({}, {}, {}, {})", obj.r, obj.g, obj.b, obj.a);

RAYLIB_DUAL_FORMATTER(
    Matrix,
    "({}, {}, {}, {} | {}, {}, {}, {} | {}, {}, {}, {} | {}, {}, {}, {})",
    obj.m0,
    obj.m1,
    obj.m2,
    obj.m3,
    obj.m4,
    obj.m5,
    obj.m6,
    obj.m7,
    obj.m8,
    obj.m9,
    obj.m10,
    obj.m11,
    obj.m12,
    obj.m13,
    obj.m14,
    obj.m15

);

RAYLIB_DUAL_FORMATTER(
    Rectangle, "({}, {}, {}, {})", obj.x, obj.y, obj.width, obj.height
);

#undef RAYLIB_DUAL_FORMATTER
#undef RAYLIB_CPP_FORMATTER
#undef RAYLIB_C_FORMATTER
