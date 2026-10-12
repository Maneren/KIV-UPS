#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>
#include <variant>

namespace match {

template <typename... Ts> struct Overloaded : Ts... {
  using Ts::operator()...;
};

template <typename... Ts> Overloaded(Ts...) -> Overloaded<std::decay_t<Ts>...>;

namespace detail {

// Check the visitor against every alternative, preserving the variant's
// value category (const-ref vs mutable-ref).
template <typename Variant, typename Visitor, std::size_t... I>
constexpr bool all_alternatives_nothrow(std::index_sequence<I...> /*unused*/) {
  return (
      std::is_nothrow_invocable_v<
          Visitor,
          decltype(std::get<I>(std::declval<Variant>()))> &&
      ...
  );
}

template <typename VariantRef, typename Visitor>
constexpr bool match_nothrow() {
  using V = std::remove_cvref_t<VariantRef>;
  constexpr std::size_t N = std::variant_size_v<V>;
  return all_alternatives_nothrow<VariantRef, Visitor>(
      std::make_index_sequence<N>{}
  );
}

} // namespace detail

/// A utility function for pattern matching on std::variant.
/// Combines multiple lambdas into a single callable object and applies them to
/// the given variant.
/// @tparam T The type of the variant.
/// @tparam Args The types of the lambdas.
/// @param t The variant to match.
/// @param args The lambdas to apply.
/// @return The result of applying the appropriate lambda to the variant.
template <typename T, typename... Args>
constexpr decltype(auto) match(T &&t, Args &&...args) noexcept(
    detail::match_nothrow<T &&, Overloaded<std::decay_t<Args>...>>()
) {
  Overloaded overload{std::forward<Args>(args)...};
  return std::visit(std::move(overload), std::forward<T>(t));
}

} // namespace match
