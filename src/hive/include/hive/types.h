#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <utils/format.h>
#include <variant>

namespace hive {

enum class Player : std::uint8_t {
  Black,
  White,
};
enum class PieceKind : std::uint8_t {
  Queen,
  Spider,
  Beetle,
  Grasshopper,
  Ant,
};
constexpr std::size_t NUMBER_OF_PIECES = 5;

struct Piece {
  PieceKind kind;
  Player owner;

  bool operator==(const Piece &other) const = default;
};

using Coordinate = std::int32_t;

struct TilePointer {
  Coordinate p;
  Coordinate q;

  bool operator==(const TilePointer &other) const = default;
};

struct MovePiece {
  TilePointer from;
  TilePointer to;
  PieceKind piece_kind;

  bool operator==(const MovePiece &other) const = default;
};

struct PlacePiece {
  TilePointer pos;
  PieceKind piece_kind;

  bool operator==(const PlacePiece &other) const = default;
};

using Move = std::variant<MovePiece, PlacePiece>;

Move make_move(TilePointer from, TilePointer to, PieceKind piece_kind);

Move make_placement(TilePointer pos, PieceKind piece_kind);

using PlayerPiecesMap = std::map<PieceKind, size_t>;

using Direction = std::pair<Coordinate, Coordinate>;

constexpr std::array<Direction, 6> DIRECTIONS{
    std::make_pair(1, 0),
    std::make_pair(0, 1),
    std::make_pair(-1, 1),
    std::make_pair(-1, 0),
    std::make_pair(0, -1),
    std::make_pair(1, -1),
};

} // namespace hive

template <> struct std::formatter<hive::Player> : utils::formatter_base {
  static auto format(auto &obj, auto &ctx) {
    const auto index = static_cast<uint_fast8_t>(obj);
    constexpr static std::array<std::string_view, 2> names{"White", "Black"};

    return std::format_to(
        ctx.out(), "{}", index < 2 ? names.at(index) : "UNKNOWN"
    );
  }
};

template <> struct std::formatter<hive::PieceKind> : utils::formatter_base {
  static auto format(const hive::PieceKind &obj, auto &ctx) {
    const auto index = static_cast<uint_fast8_t>(obj);
    constexpr static std::array<std::string_view, hive::NUMBER_OF_PIECES> names{
        "Queen",
        "Spider",
        "Beetle",
        "Grasshopper",
        "Ant",
    };

    return std::format_to(
        ctx.out(),
        "{}",
        index < hive::NUMBER_OF_PIECES ? names.at(index) : "Unknown"
    );
  }
};

template <> struct std::formatter<hive::Piece> : utils::formatter_base {
  static auto format(const hive::Piece &obj, auto &ctx) {
    if (obj.owner == hive::Player::White) {
      auto str = std::format("{}", obj.kind);
      std::ranges::transform(str, str.begin(), ::toupper);
      return std::format_to(ctx.out(), "{}", str);
    }

    return std::format_to(ctx.out(), "{}", obj.kind);
  }
};

template <> struct std::hash<hive::TilePointer> {
  std::size_t operator()(auto p) const {
    return std::hash<int>()(p.p) ^ std::hash<int>()(p.q);
  }
};

template <> struct std::formatter<hive::TilePointer> : utils::formatter_base {
  static auto format(const hive::TilePointer &obj, auto &ctx) {
    return std::format_to(ctx.out(), "({}, {})", obj.p, obj.q);
  }
};

template <> struct std::formatter<hive::MovePiece> : utils::formatter_base {
  static auto format(const hive::MovePiece &obj, auto &ctx) {
    return std::format_to(
        ctx.out(), "{}: {} -> {}", obj.piece_kind, obj.from, obj.to
    );
  }
};

template <> struct std::formatter<hive::PlacePiece> : utils::formatter_base {
  static auto format(const hive::PlacePiece &obj, auto &ctx) {
    return std::format_to(ctx.out(), "{}: {}", obj.piece_kind, obj.pos);
  }
};

template <> struct std::formatter<hive::Move> : utils::formatter_base {
  static auto format(const hive::Move &obj, auto &ctx) {
    return std::visit(
        [&ctx](const auto &move) {
          return std::format_to(ctx.out(), "{}", move);
        },
        obj
    );
  }
};
