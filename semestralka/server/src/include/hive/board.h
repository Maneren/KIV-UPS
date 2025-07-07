#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <utils/generator.h>
#include <vector>

namespace hive {

enum class Player : std::uint8_t { Black, White };
enum class PieceKind : std::uint8_t { Queen, Spider, Beetle, Grasshopper, Ant };
constexpr std::size_t NUMBER_OF_PIECES = 5;
struct Piece {
  PieceKind piece;
  Player owner;
};

struct TilePointer {
  int p;
  int q;

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

constexpr std::array<std::pair<int, int>, 6> DIRECTIONS{
    std::make_pair(1, 0),
    std::make_pair(0, 1),
    std::make_pair(-1, 1),
    std::make_pair(-1, 0),
    std::make_pair(0, -1),
    std::make_pair(1, -1),
};

inline std::pair<int, int> rotate_left(std::pair<int, int> dir) {
  return {dir.first + dir.second, -dir.first};
}
inline std::pair<int, int> rotate_right(std::pair<int, int> dir) {
  return {-dir.second, dir.first + dir.second};
}

class Board {
public:
  class LiftPiece {
    TilePointer ptr;
    Piece piece;
    Board *board;

  public:
    LiftPiece(TilePointer ptr, Board *board)
        : ptr(ptr), piece(board->remove_piece(ptr)), board(board) {}

    ~LiftPiece() { board->add_piece(ptr, piece); }

    LiftPiece(const LiftPiece &) = delete;
    LiftPiece(LiftPiece &&) = default;
    LiftPiece &operator=(const LiftPiece &) = delete;
    LiftPiece &operator=(LiftPiece &&) = default;
  };

  [[nodiscard]] const std::vector<Piece> &get(TilePointer ptr) const {
    return data.at(ptr);
  }

  [[nodiscard]] Piece get_top(TilePointer ptr) const { return get(ptr).back(); }

  [[nodiscard]] bool is_empty() const { return data.empty(); }
  [[nodiscard]] bool is_empty(TilePointer ptr) const {
    try {
      return data.at(ptr).empty();
    } catch (const std::out_of_range &) {
      return true;
    }
  }

  [[nodiscard]] static Board from_fen_string(std::string_view fen);
  [[nodiscard]] std::string to_fen_string() const;

  std::generator<std::pair<TilePointer, Piece>>
  players_tiles(Player player) const;

  void add_piece(TilePointer ptr, Piece piece);

  Piece remove_piece(TilePointer ptr);

  std::generator<Move> moves_for_player(Player player, PlayerPiecesMap pieces);

  static std::generator<TilePointer> neighboring_cells(TilePointer ptr);

  std::generator<TilePointer> neighbors(TilePointer ptr) const;

  std::generator<TilePointer> empty_neighbors(TilePointer ptr) const;

  bool tile_belongs_to_player(TilePointer ptr, Player player) const;

  bool neighbors_only_players(TilePointer ptr, Player player) const;

  std::unordered_set<TilePointer, TilePointerHasher> tiles_around_hive() const;

  std::generator<TilePointer> valid_placements(Player player) const;

  bool moving_breaks_hive(TilePointer ptr);

  std::generator<std::pair<TilePointer, Piece>>
  moveable_pieces_for(Player player);

  bool has_neighbor(TilePointer cell) const;

  bool has_neighbor_in_direction(
      TilePointer cell, std::pair<int, int> direction
  ) const;

  bool can_move_to(TilePointer from, TilePointer to, bool can_leave) const;

  std::generator<TilePointer>
  valid_steps(TilePointer ptr, bool can_leave = false) const;

  std::generator<Move> queens_moves(TilePointer queen);

  std::generator<Move> beetle_moves(TilePointer beetle);

  std::generator<Move> grasshopper_moves(TilePointer grasshopper);

  std::generator<Move> spider_moves(TilePointer spider);

  std::generator<Move> ant_moves(TilePointer ant);

private:
  std::unordered_map<TilePointer, std::vector<Piece>, TilePointerHasher> data;
};

} // namespace hive
