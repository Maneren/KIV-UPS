#pragma once

#include "types.h"
#include <format>
#include <utils/format.h>
#include <utils/serde.h>

namespace hive {

class Message {
public:
  Message() = default;
  Message(const Message &) = default;
  Message(Message &&) = default;
  Message &operator=(const Message &) = default;
  Message &operator=(Message &&) = default;

  virtual ~Message() = default;

  bool operator==(const Message &other) const = default;
};

class MoveMessage : Message {
  Move _move;
  Player _player;

public:
  MoveMessage(Move move, Player player) : _move(move), _player(player) {}

  bool operator==(const MoveMessage &other) const {
    return _move == other._move && _player == other._player;
  }

  [[nodiscard]] Move move() const { return _move; }
  [[nodiscard]] Player player() const { return _player; }
};

class OkMessage : Message {
public:
  OkMessage() = default;

  bool operator==(const OkMessage & /*unused*/) const { return true; }
};

} // namespace hive

template <typename O> struct serde::serializer<hive::TilePointer, O> {
  static auto serialize_to(auto msg, auto ctx) {
    serde::serialize_to(msg.p, std::move(ctx));
    ctx.out() = ',';
    serde::serialize_to(msg.q, std::move(ctx));
    return ctx;
  }
};

template <> struct serde::deserializer<hive::TilePointer> {
  static std::optional<hive::TilePointer> deserialize(auto &ctx) {
    auto p = serde::deserialize<int>(ctx);

    if (!ctx.assert_prefix(",")) {
      return std::nullopt;
    }

    auto q = serde::deserialize<int>(ctx);

    return hive::TilePointer{.p = *p, .q = *q};
  }
};

template <typename O> struct serde::serializer<hive::Player, O> {
  static auto serialize_to(auto msg, auto ctx) {
    switch (msg) {
    case hive::Player::Black:
      return std::format_to(ctx.out(), "B");
    case hive::Player::White:
      return std::format_to(ctx.out(), "W");
    }
  }
};

template <> struct serde::deserializer<hive::Player> {
  static std::optional<hive::Player> deserialize(auto &ctx) {
    if (ctx.assert_prefix("B")) {
      return hive::Player::Black;
    }

    if (ctx.assert_prefix("W")) {
      return hive::Player::White;
    }

    return std::nullopt;
  }
};

template <typename O> struct serde::serializer<hive::PieceKind, O> {
  static auto serialize_to(auto &msg, auto ctx) {
    switch (msg) {
    case hive::PieceKind::Queen:
      return std::format_to(ctx.out(), "Q");
    case hive::PieceKind::Grasshopper:
      return std::format_to(ctx.out(), "G");
    case hive::PieceKind::Beetle:
      return std::format_to(ctx.out(), "B");
    case hive::PieceKind::Spider:
      return std::format_to(ctx.out(), "S");
    case hive::PieceKind::Ant:
      return std::format_to(ctx.out(), "A");
    }
  }
};

template <> struct serde::deserializer<hive::PieceKind> {
  static std::optional<hive::PieceKind> deserialize(auto &ctx) {
    if (ctx.assert_prefix("Q")) {
      return hive::PieceKind::Queen;
    }

    if (ctx.assert_prefix("G")) {
      return hive::PieceKind::Grasshopper;
    }

    if (ctx.assert_prefix("B")) {
      return hive::PieceKind::Beetle;
    }

    if (ctx.assert_prefix("S")) {
      return hive::PieceKind::Spider;
    }

    if (ctx.assert_prefix("A")) {
      return hive::PieceKind::Ant;
    }

    return std::nullopt;
  }
};

template <typename O> struct serde::serializer<hive::MoveMessage, O> {
  template <typename T> using ser = serde::serializer<T, O>;
  static auto serialize_to(auto &msg, auto ctx) {
    auto [from, to, piece_kind] = msg.move();
    std::format_to(ctx.out(), "m;");
    serde::serialize_to(from, std::move(ctx));
    std::format_to(ctx.out(), ";");
    serde::serialize_to(to, std::move(ctx));
    std::format_to(ctx.out(), ";");
    serde::serialize_to(piece_kind, std::move(ctx));
    std::format_to(ctx.out(), ";");
    serde::serialize_to(msg.player(), std::move(ctx));
    std::format_to(ctx.out(), ".");
    return ctx;
  }
};

#include <utils/print.h>
template <> struct serde::deserializer<hive::MoveMessage> {
  static std::optional<hive::MoveMessage> deserialize(auto &ctx) {
    if (!ctx.assert_prefix("m;")) {
      return std::nullopt;
    }

    auto from = serde::deserialize<hive::TilePointer>(ctx);

    if (!ctx.assert_prefix(";")) {
      return std::nullopt;
    }

    auto to = serde::deserialize<hive::TilePointer>(ctx);

    if (!ctx.assert_prefix(";")) {
      return std::nullopt;
    }

    auto piece_kind = serde::deserialize<hive::PieceKind>(ctx);

    if (!ctx.assert_prefix(";")) {
      return std::nullopt;
    }

    auto player = serde::deserialize<hive::Player>(ctx);

    auto move = hive::Move{.from = *from, .to = *to, .piece_kind = *piece_kind};

    if (!ctx.assert_prefix(".")) {
      return std::nullopt;
    }

    return hive::MoveMessage{move, *player};
  }
};

template <typename O> struct serde::serializer<hive::OkMessage, O> {
  static auto serialize_to(auto /*unused*/, auto ctx) {
    return std::format_to(ctx.out(), "k.");
  }
};

template <> struct serde::deserializer<hive::OkMessage> {
  static std::optional<hive::OkMessage>
  deserialize(DeserializationContext &ctx) {
    if (!ctx.assert_prefix("k.")) {
      return std::nullopt;
    }

    return hive::OkMessage{};
  }
};
