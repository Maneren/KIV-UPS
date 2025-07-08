#include <array>
#include <cstdint>
#include <format>
#include <functional>

namespace hive {

enum class Player : std::uint8_t { Black, White };
enum class PieceKind : std::uint8_t { Queen, Spider, Beetle, Grasshopper, Ant };
constexpr std::size_t NUMBER_OF_PIECES = 5;
struct Piece {
  PieceKind piece;
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
  PieceKind piece;
};

struct TilePointerHasher {
  std::size_t operator()(TilePointer p) const {
    return std::hash<int>()(p.p) ^ std::hash<int>()(p.q);
  }
};

using PlayerPiecesMap = std::array<std::uint8_t, NUMBER_OF_PIECES>;

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

template <> struct std::formatter<hive::TilePointer> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(auto &obj, std::format_context &ctx) {
    return std::format_to(ctx.out(), "({}, {})", obj.p, obj.q);
  }
};
