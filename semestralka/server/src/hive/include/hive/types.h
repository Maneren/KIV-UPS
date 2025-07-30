#pragma once

#include <array>
#include <cstdint>
#include <format>
#include <functional>
#include <map>

namespace hive {

enum class Player : std::uint8_t { Black, White };
enum class PieceKind : std::uint8_t { Queen, Spider, Beetle, Grasshopper, Ant };
constexpr std::size_t NUMBER_OF_PIECES = 5;
struct Piece {
  PieceKind kind;
  Player owner;
};

using Coordinate = int;

struct TilePointer {
  Coordinate p;
  Coordinate q;

  bool operator==(const TilePointer &other) const = default;
};

struct Move {
  TilePointer from;
  TilePointer to;
  PieceKind piece_kind;
};

inline Move make_move(TilePointer from, TilePointer to, PieceKind piece_kind) {
  return Move{.from = from, .to = to, .piece_kind = piece_kind};
}

inline Move make_placement(TilePointer pos, PieceKind piece_kind) {
  return Move{.from = pos, .to = pos, .piece_kind = piece_kind};
}

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

template <> struct std::formatter<hive::PieceKind> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    switch (obj) {
    case hive::PieceKind::Queen:
      return std::format_to(ctx.out(), "Queen");
    case hive::PieceKind::Spider:
      return std::format_to(ctx.out(), "Spider");
    case hive::PieceKind::Beetle:
      return std::format_to(ctx.out(), "Beetle");
    case hive::PieceKind::Grasshopper:
      return std::format_to(ctx.out(), "Grasshopper");
    case hive::PieceKind::Ant:
      return std::format_to(ctx.out(), "Ant");
    default:
      return std::format_to(ctx.out(), "Unknown Piece");
    }
  }
};

template <> struct std::formatter<hive::Piece> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }
  static auto format(auto &obj, std::format_context &ctx) {
    if (obj.owner == hive::Player::White) {
      auto str = std::format("{}", obj.kind);
      std::transform(str.begin(), str.end(), str.begin(), ::toupper);
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

template <> struct std::formatter<hive::TilePointer> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    return std::format_to(ctx.out(), "({}, {})", obj.p, obj.q);
  }
};

template <> struct std::formatter<hive::Move> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    if (obj.from != obj.to) {
      return std::format_to(
          ctx.out(), "{}: {} -> {}", obj.piece_kind, obj.from, obj.to
      );
    }

    return std::format_to(ctx.out(), "{}: {}", obj.piece_kind, obj.to);
  }
};
