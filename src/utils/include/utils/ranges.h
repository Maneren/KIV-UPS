#pragma once

#include <ranges>
#include <tuple>

namespace utils::views {

namespace detail {
struct enumerate_uz_impl {
  template <std::ranges::input_range Rng>
  constexpr auto operator()(Rng &&rng) const {
    return std::views::transform(
        std::views::enumerate(std::forward<Rng>(rng)), [](auto &&p) {
          return std::make_tuple(
              static_cast<size_t>(std::get<0>(p)), std::get<1>(p)
          );
        }
    );
  }

  friend constexpr auto
  operator|(std::ranges::input_range auto &&rng, enumerate_uz_impl fn) {
    return fn(std::forward<decltype(rng)>(rng));
  }
};
} // namespace detail

inline constexpr detail::enumerate_uz_impl enumerate_uz;

} // namespace utils::views
