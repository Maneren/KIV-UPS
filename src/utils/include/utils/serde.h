#pragma once

#include <algorithm>
#include <charconv>
#include <format>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <utils/print.h>
#include <vector>

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
   * @param prefix Prefix to check for
   * @return true if the prefix was found, false otherwise
   */
  [[nodiscard]] bool assert_prefix(const std::basic_string_view<C> prefix) {
    const auto ok = std::ranges::equal(
        prefix, std::basic_string_view<C>(this->begin(), prefix.size())
    );

    if (ok) {
      this->remove_prefix(prefix.size());
    }

    return ok;
  }
};

using DeserializationContext = BaseDeserializationContext<char>;

template <typename O, typename C> class BaseSerializationContext {
  static_assert(std::output_iterator<O, C>);
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

  [[nodiscard]] iterator out() const { return std::move(_out); }
};

template <typename I>
BaseSerializationContext(I)
    -> BaseSerializationContext<I, typename I::container_type::value_type>;

template <typename O>
using SerializationContext = BaseSerializationContext<O, char>;

template <typename T, std::output_iterator<char> O> struct serializer {
  static SerializationContext<O>
  serialize_to(const T &obj, SerializationContext<O> &&out);
};

template <typename T, typename C = char> struct deserializer {
  static std::optional<T> deserialize(BaseDeserializationContext<C> &ctx);
};

template <typename O = std::string> O serialize(const auto &obj) {
  O str;
  auto it = std::back_inserter(str);
  BaseSerializationContext ctx(it);
  serializer<std::remove_cvref_t<decltype(obj)>, decltype(it)>::serialize_to(
      obj, std::move(ctx)
  );
  return str;
}

void serialize_to(const auto &obj, auto &&ctx) {
  serializer<
      std::remove_cvref_t<decltype(obj)>,
      typename std::remove_cvref_t<decltype(ctx)>::iterator>::
      serialize_to(obj, std::forward<decltype(ctx)>(ctx));
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

template <typename O> struct serializer<int, O> {
  static auto serialize_to(const int &obj, auto out) {
    return std::format_to(out.out(), "{}", obj);
  }
};

template <typename O> struct serializer<std::string, O> {
  static auto serialize_to(const std::string &obj, auto out) {
    return std::format_to(out.out(), "\"{}\"", obj);
  }
};

template <typename O> struct serializer<bool, O> {
  static auto serialize_to(const bool &obj, auto out) {
    return std::format_to(out.out(), "{}", obj ? "T" : "F");
  }
};

template <typename O> struct serializer<double, O> {
  static auto serialize_to(const double &obj, auto out) {
    return std::format_to(out.out(), "{}", obj);
  }
};

template <typename O> struct serializer<float, O> {
  static auto serialize_to(const float &obj, auto out) {
    return std::format_to(out.out(), "{}", obj);
  }
};

template <typename O, typename T> struct serializer<std::vector<T>, O> {
  static auto serialize_to(const std::vector<int> &obj, auto out) {
    auto it = out.out();
    // Format: [size, elem1, elem2,...]
    *it = std::format_to(it, "[{}", obj.size());

    for (const auto &elem : obj) {
      *it++ = ',';
      *it = serde::serializer<T, O>::serialize_to(
          elem, BaseSerializationContext{it}
      );
    }
    *it++ = ']';

    return O{it};
  }
};

template <> struct deserializer<std::string> {
  static std::optional<std::string> deserialize(auto &ctx) {
    if (!ctx.assert_prefix("\"")) {
      return std::nullopt;
    }

    auto length = std::ranges::distance(
        std::ranges::begin(ctx), std::ranges::find(ctx, '"')
    );

    auto res = std::string(ctx.begin(), ctx.begin() + length);
    ctx.remove_prefix(static_cast<size_t>(length));

    if (!ctx.assert_prefix("\"")) {
      return std::nullopt;
    }

    return res;
  }
};

template <> struct deserializer<bool> {
  static std::optional<bool> deserialize(auto &ctx) {
    if (ctx.assert_prefix("T")) {
      return true;
    }

    if (ctx.assert_prefix("F")) {
      return false;
    }

    return std::nullopt;
  }
};

template <typename T> struct deserializer<T> {
  static std::optional<T> deserialize(auto &ctx) {
    T value;

    if (auto result = std::from_chars(ctx.data(), ctx.end(), value);
        result.ec == std::errc{}) {
      ctx.remove_prefix(static_cast<size_t>(result.ptr - ctx.data()));
      return std::make_optional(value);
    }

    return std::nullopt;
  }
};

template <typename T> struct deserializer<std::vector<T>> {
  static std::optional<std::vector<T>> deserialize(auto &ctx) {
    if (!ctx.assert_prefix("[")) {
      return std::nullopt;
    }

    auto close_bracket = std::ranges::find(ctx, ']');
    if (close_bracket == ctx.end()) {
      return std::nullopt;
    }

    ctx.remove_suffix(1);

    size_t size = 0;
    auto size_result = std::from_chars(ctx.begin(), ctx.end(), size);

    if (size_result.ec != std::errc{}) {
      std::println("Error: from_chars: {}", static_cast<int>(size_result.ec));
      return std::nullopt;
    }

    ctx.remove_prefix(1 + static_cast<size_t>(size_result.ptr - ctx.begin()));

    std::println("ctx: {}", ctx.data());

    std::vector<int> result;

    if (size == 0) {
      return result;
    }

    result.reserve(size);

    try {
      decltype(ctx.end()) end;
      while ((end = std::ranges::find(ctx, ',')) != ctx.end()) {
        auto token = std::string_view(ctx.begin(), end);
        ctx.remove_prefix(token.size() + 1);
        result.push_back(*serde::deserialize<T>(token));
      }

      result.push_back(*serde::deserialize<T>(ctx));

      if (result.size() != size) {
        return std::nullopt;
      }

    } catch (const std::exception &) {
      return std::nullopt;
    }

    return result;
  }
};

// template <typename C> class Sink_iter {
//   Sink<C> *mSink = nullptr;
//
// public:
//   using iterator_category = std::output_iterator_tag;
//   using value_type = void;
//   using difference_type = ptrdiff_t;
//   using pointer = void;
//   using reference = void;
//
//   Sink_iter() = default;
//   Sink_iter(const Sink_iter &) = default;
//   Sink_iter &operator=(const Sink_iter &) = default;
//
//   explicit constexpr Sink_iter(Sink<C> &sink) : mSink(std::addressof(sink))
//   {} constexpr Sink_iter &operator=(C c) {
//     mSink->_M_write(c);
//     return *this;
//   }
//   constexpr Sink_iter &operator=(std::basic_string_view<C> s) {
//     mSink->_M_write(s);
//     return *this;
//   }
//
//   constexpr Sink_iter &operator*() { return *this; }
//
//   constexpr Sink_iter &operator++() { return *this; }
//
//   constexpr Sink_iter operator++(int) { return *this; }
//
//   auto m_reserve(size_t n) const { return mSink->_M_reserve(n); }
// };
//
// template <typename C> class Sink {
//   friend class Sink_iter<C>;
//
//   std::span<C> _span;
//   typename std::span<C>::iterator _next;
//
//   virtual void m_overflow() = 0;
//
// protected:
//   explicit constexpr Sink(std::span<C> span) noexcept
//       : _span(span), _next(span.begin()) {}
//
//   std::span<C> _used() const noexcept {
//     return _span.first(_next - _span.begin());
//   }
//
//   // The portion of the span that has not been written to.
//   constexpr std::span<C> _unused() const noexcept {
//     return _span.subspan(_next - _span.begin());
//   }
//
//   // Use the start of the span as the next write position.
//   constexpr void _rewind() noexcept { _next = _span.begin(); }
//
//   // Replace the current output range.
//   void _reset(std::span<C> s, size_t pos = 0) noexcept {
//     _span = s;
//     _next = s.begin() + pos;
//   }
//
//   // Called by the iterator for *it++ = c
//   constexpr void _write(C c) {
//     *_next++ = c;
//     if (_next - _span.begin() == std::ssize(_span)) {
//       [[unlikely]] m_overflow();
//     }
//   }
//
//   constexpr void _write(std::basic_string_view<C> s) {
//     std::span to = _unused();
//     while (to.size() <= s.size()) {
//       s.copy(to.data(), to.size());
//       _next += to.size();
//       s.remove_prefix(to.size());
//       m_overflow();
//       to = _unused();
//     }
//     if (s.size()) {
//       s.copy(to.data(), s.size());
//       _next += s.size();
//     }
//   }
//
//   // A successful _Reservation can be used to directly write
//   // up to N characters to the sink to avoid unwanted buffering.
//   struct Reservation {
//     // True if the reservation was successful, false otherwise.
//     explicit operator bool() const noexcept { return _sink; }
//     // A pointer to write directly to the sink.
//     C *get() const noexcept { return _sink->_next.operator->(); }
//     // Add n to the _M_next iterator for the sink.
//     void _bump(size_t n) { _sink->_bump(n); }
//     Sink *_sink;
//   };
//
//   // Attempt to reserve space to write n characters to the sink.
//   // If anything is written to the reservation then there must be a call
//   // to _M_bump(N2) before any call to another member function of *this,
//   // where N2 is the number of characters written.
//   virtual Reservation _reserve(size_t n) {
//     if (n <= _unused().size()) {
//       return {this};
//     }
//
//     if (n <= _span.size()) // Cannot meet the request.
//     {
//       m_overflow(); // Make more space available.
//       if (n <= _unused().size()) {
//         return {this};
//       }
//     }
//     return {nullptr};
//   }
//
//   virtual void _bump(size_t n) { _next += n; }
//
// public:
//   Sink(const Sink &) = delete;
//   Sink &operator=(const Sink &) = delete;
//
//   constexpr Sink_iter<C> out() noexcept { return Sink_iter<C>(*this); }
// };

} // namespace serde
