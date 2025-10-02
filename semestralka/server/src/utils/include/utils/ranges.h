#pragma once

/// Adapted from https://stackoverflow.com/a/79303194

#include <ranges>

namespace {
template <typename Container> struct collect_impl {
  auto operator()(std::ranges::input_range auto &&range) const {
    auto r = range | std::views::common;
    return Container(std::ranges::begin(r), std::ranges::end(r));
  }

  friend auto operator|(std::ranges::input_range auto &&range, collect_impl c) {
    return c(range);
  }
};

template <template <typename...> typename Template, typename... T>
concept well_formed_template = requires() { typename Template<T...>; };

template <template <typename...> typename Container> struct auto_collect {
  template <std::ranges::input_range Rng> auto operator()(Rng &&range) const {
    auto r = range | std::views::common;
    using element_t = std::ranges::range_value_t<Rng>;
    if constexpr (well_formed_template<Container, element_t>) {
      return Container<element_t>(std::ranges::begin(r), std::ranges::end(r));
    } else {
      using first_t = decltype(std::declval<element_t>().first);
      using second_t = decltype(std::declval<element_t>().second);
      static_assert(well_formed_template<Container, first_t, second_t>);
      return Container<first_t, second_t>(
          std::ranges::begin(r), std::ranges::end(r)
      );
    }
  }

  friend auto operator|(std::ranges::input_range auto &&range, auto_collect c) {
    return c(range);
  }
};
} // namespace

namespace ranges {

template <typename Container> constexpr auto collect() {
  return collect_impl<Container>{};
}

template <template <typename...> typename Container> constexpr auto collect() {
  return auto_collect<Container>{};
}

template <typename Container>
constexpr auto collect(std::ranges::input_range auto &&rng) {
  auto collector = collect_impl<Container>{};
  return collector(rng);
}

template <template <typename...> typename Container>
constexpr auto collect(std::ranges::input_range auto &&rng) {
  auto collector = auto_collect<Container>{};
  return collector(rng);
}

} // namespace ranges
