#pragma once

/// Adapted from https://stackoverflow.com/a/79303194

#include <ranges>
#include <tuple>
#include <type_traits>

namespace utils {

namespace ranges {
namespace detail {

template <typename Container> struct collect_impl {
  auto operator()(std::ranges::input_range auto &&range) const {
    auto r = range | std::views::common;
    return Container(std::ranges::begin(r), std::ranges::end(r));
  }

  friend auto operator|(std::ranges::input_range auto &&range, collect_impl c) {
    return c(std::forward<decltype(range)>(range));
  }
};

template <template <typename...> typename Template, typename... T>
concept well_formed_template = requires() { typename Template<T...>; };

template <template <typename...> typename Container> struct auto_collect_impl {
  template <std::ranges::input_range Rng> auto operator()(Rng &&range) const {
    auto r = std::forward<Rng>(range) | std::views::common;
    using element_t = std::ranges::range_value_t<Rng>;
    if constexpr (well_formed_template<Container, element_t>) {
      return Container<element_t>(std::ranges::begin(r), std::ranges::end(r));
    } else {
      // Only works for 2-tuples
      static_assert(requires {
        typename std::tuple_size<element_t>::type;
      } && std::tuple_size_v<element_t> == 2);

      using first_t =
          std::remove_cvref_t<decltype(std::get<0>(std::declval<element_t>()))>;
      using second_t =
          std::remove_cvref_t<decltype(std::get<1>(std::declval<element_t>()))>;

      static_assert(well_formed_template<Container, first_t, second_t>);

      return Container<first_t, second_t>(
          std::ranges::begin(r), std::ranges::end(r)
      );
    }
  }

  friend auto
  operator|(std::ranges::input_range auto &&range, auto_collect_impl c) {
    return c(std::forward<decltype(range)>(range));
  }
};
} // namespace detail

template <typename Container> constexpr auto collect() {
  return detail::collect_impl<Container>{};
}

template <template <typename...> typename Container> constexpr auto collect() {
  return detail::auto_collect_impl<Container>{};
}

template <typename Container>
constexpr auto collect(std::ranges::input_range auto &&rng) {
  return detail::collect_impl<Container>{}(std::forward<decltype(rng)>(rng));
}

template <template <typename...> typename Container>
constexpr auto collect(std::ranges::input_range auto &&rng) {
  return detail::auto_collect_impl<Container>{}(
      std::forward<decltype(rng)>(rng)
  );
}

} // namespace ranges

namespace views {
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
} // namespace views

} // namespace utils
