#pragma once

#include <utility>

namespace functional {

template <typename T> struct BindConstructor {
  template <typename... Args> T operator()(Args &&...args) const {
    return T(std::forward<Args>(args)...);
  }
};

inline void drop(...) {}

} // namespace functional
