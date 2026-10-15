#pragma once

#include <format>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace serde {

template <typename C>
class BaseDeserializationContext : public std::basic_string_view<C> {
public:
  using std::basic_string_view<C>::basic_string_view;

  BaseDeserializationContext(std::basic_string_view<C> str)
      : std::basic_string_view<C>(str) {}
  BaseDeserializationContext(const std::string &str)
      : std::basic_string_view<C>(str) {}

  /**
   * @brief Checks if the string starts with the given prefix and removes it
   *
   * Bounds-safe: a prefix longer than the remaining input simply fails.
   *
   * @param prefix Prefix to check for
   * @return true if the prefix was found, false otherwise
   */
  [[nodiscard]] bool assert_prefix(const std::basic_string_view<C> prefix) {
    if (!this->starts_with(prefix)) {
      return false;
    }
    this->remove_prefix(prefix.size());
    return true;
  }
};

using DeserializationContext = BaseDeserializationContext<char>;

/**
 * @brief RAII guard for atomic (all-or-nothing) parsing.
 *
 * Snapshot the context on construction and restore it on destruction
 * unless commit() was called.
 */
template <typename C> class DeserializationTransaction {
public:
  explicit DeserializationTransaction(BaseDeserializationContext<C> &ctx)
      : ctx(ctx), saved(ctx) {}
  DeserializationTransaction(const DeserializationTransaction &) = delete;
  DeserializationTransaction &
  operator=(const DeserializationTransaction &) = delete;
  DeserializationTransaction(DeserializationTransaction &&) = delete;
  DeserializationTransaction &operator=(DeserializationTransaction &&) = delete;

  void commit() noexcept { committed = true; }

  ~DeserializationTransaction() {
    if (!committed) {
      ctx = saved;
    }
  }

private:
  BaseDeserializationContext<C> &ctx;
  BaseDeserializationContext<C> saved;
  bool committed = false;
};

template <typename O, typename C>
  requires std::output_iterator<O, C>
class BaseSerializationContext {
  O _out;

public:
  BaseSerializationContext(O out) : _out(std::move(out)) {}
  BaseSerializationContext(BaseSerializationContext &) = delete;
  BaseSerializationContext &operator=(BaseSerializationContext &) = delete;
  BaseSerializationContext &
  operator=(BaseSerializationContext &&other) noexcept {
    _out = std::move(other._out);
    return *this;
  };
  BaseSerializationContext(BaseSerializationContext &&other) noexcept
      : _out(std::move(other._out)) {}

  ~BaseSerializationContext() = default;

  using char_type = C;
  using iterator = O;

  [[nodiscard]] iterator out() { return _out; }
};

template <typename I>
BaseSerializationContext(I)
    -> BaseSerializationContext<I, typename I::container_type::value_type>;

template <typename O>
using SerializationContext = BaseSerializationContext<O, char>;

template <typename T, typename O, typename C = char> struct serializer {
  static auto
  serialize_to(const T &obj, BaseSerializationContext<O, C> &out) = delete;
};

template <typename T, typename C = char> struct deserializer {
  static std::optional<T>
  deserialize(BaseDeserializationContext<C> &ctx) = delete;
};

template <typename O = std::string> O serialize(const auto &obj) {
  O str;
  auto it = std::back_inserter(str);
  BaseSerializationContext ctx(it);
  serializer<std::remove_cvref_t<decltype(obj)>, decltype(it)>::serialize_to(
      obj, ctx
  );
  return str;
}

auto serialize_to(const auto &obj, auto &ctx) {
  return serializer<
      std::remove_cvref_t<decltype(obj)>,
      typename std::remove_cvref_t<decltype(ctx)>::iterator>::
      serialize_to(obj, ctx);
}

template <typename T, typename C = char>
std::optional<T> deserialize(std::basic_string_view<C> data) {
  BaseDeserializationContext<C> ctx(data);
  return deserializer<T, C>::deserialize(ctx);
}

template <typename T, typename C = char>
std::optional<T> deserialize(std::basic_string<C> data) {
  BaseDeserializationContext<C> ctx(data);
  return deserializer<T, C>::deserialize(ctx);
}

template <typename T, typename C = char>
std::optional<T> deserialize(BaseDeserializationContext<C> &ctx) {
  return deserializer<T, C>::deserialize(ctx);
}

/**
 * @brief Deserialize T and require the entire input to be consumed.
 *
 * Unlike deserialize, remaining input fails the parse.
 */
template <typename T, typename C = char>
std::optional<T> deserialize_full(std::basic_string_view<C> data) {
  BaseDeserializationContext<C> ctx(data);
  auto result = deserializer<T, C>::deserialize(ctx);
  if (!result.has_value() || !ctx.empty()) {
    return std::nullopt;
  }
  return result;
}

template <typename T, std::size_t N>
std::optional<T>
read_keyword(auto &ctx, const std::pair<std::string_view, T> (&table)[N]) {
  for (const auto &[text, value] : table) {
    if (ctx.assert_prefix(text)) {
      return value;
    }
  }
  return std::nullopt;
}

/**
 * @brief Write the keyword spelling mapped to value in table.
 *
 * Counterpart of read_keyword for serializers; takes and returns the
 * output position exactly like std::format_to, so calls chain. Writes
 * nothing (returns the input position) when value has no table entry;
 * all mapped values should be covered.
 */
template <typename T, std::size_t N>
auto write_keyword(
    const T &value, auto it, const std::pair<std::string_view, T> (&table)[N]
) {
  for (const auto &[text, entry] : table) {
    if (entry == value) {
      return std::format_to(it, "{}", text);
    }
  }
  return it;
}

} // namespace serde
