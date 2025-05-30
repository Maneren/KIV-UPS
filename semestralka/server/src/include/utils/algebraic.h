#pragma once

#include <cassert>
#include <type_traits>

namespace algebraic {

// adapted from https://gist.github.com/bzar/00765d257322cf77309b

template <typename C, typename F, typename R = std::invoke_result_t<F>>
R match(C const &, F f) {
  return f();
}
template <typename C, typename R> R match(C const &) { assert(false); }

template <
    typename C,
    typename T,
    typename F,
    typename R = std::invoke_result_t<F, T &>,
    typename... Args>
R match(C const &c, T C::*tp, F f, Args... args) {
  if (c.type == (c.*tp).type) {
    return f(c.*tp);
  } else if constexpr (sizeof...(args) >= 3) {
    return match<C>(c, args...);
  } else if constexpr (sizeof...(args) == 2) {
    return match<C>(c, args...);
  } else if constexpr (sizeof...(args) == 1) {
    static_assert(false);
  } else {
    return match<C, R>(c);
  }
}

} // namespace algebraic
