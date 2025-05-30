#pragma once

/*
 * print.h
 *
 * Polyfill for C++23 std::print and std::println.
 *
 * Requires C++20 for std::format and std::format_to.
 */

#include <ostream>

#ifdef __cpp_lib_print // C++ >= 23

#include <print>

#else

#include <format>
#include <iostream>
#include <utility>

namespace std {

template <typename... Args>
inline void
print(std::ostream &ostream, std::format_string<Args...> fmt, Args &&...args) {
  std::ostreambuf_iterator<char> out(ostream);
  std::format_to(out, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void print(std::format_string<Args...> fmt, Args &&...args) {
  std::print(std::cout, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void println(
    std::ostream &ostream, std::format_string<Args...> fmt, Args &&...args
) {
  std::print(ostream, "{}\n", std::format(fmt, std::forward<Args>(args)...));
}

template <typename... Args>
inline void println(std::format_string<Args...> fmt, Args &&...args) {
  std::println(std::cout, fmt, std::forward<Args>(args)...);
}

} // namespace std

#endif
