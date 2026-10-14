#pragma once

#include <generator>
#include <hive/types.h>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <utils/format.h>
#include <vector>

namespace hive {

Direction rotate_left(Direction dir);
Direction rotate_right(Direction dir);

class Board {
public:
  [[nodiscard]] const std::vector<Piece> &get(TilePointer ptr) const;

  [[nodiscard]] Piece
  get_top(TilePointer ptr, std::optional<TilePointer> ignored = {}) const;

  [[nodiscard]] bool is_empty() const;
  [[nodiscard]] bool
  is_empty(TilePointer ptr, std::optional<TilePointer> ignored = {}) const;

  [[nodiscard]] bool has_placed(Player player) const;
  [[nodiscard]] bool has_placed_queen(Player player) const;

  [[nodiscard]] std::generator<std::pair<TilePointer, Piece>>
  pieces(std::optional<TilePointer> ignored = {}) const;

  [[nodiscard]] std::generator<std::pair<TilePointer, Piece>>
  players_tiles(Player player, std::optional<TilePointer> ignored = {}) const;

  void add_piece(TilePointer ptr, Piece piece);

  Piece remove_piece(TilePointer ptr);

  [[nodiscard]]
  std::generator<Move> moves_for_piece(TilePointer pos, Piece piece) const;

  [[nodiscard]] std::generator<Move>
  moves_for_player(Player player, PlayerPiecesMap pieces) const;

  [[nodiscard]] std::generator<TilePointer>
  neighbors(TilePointer ptr, std::optional<TilePointer> ignored = {}) const;

  [[nodiscard]] std::generator<TilePointer> empty_neighbors(
      TilePointer ptr, std::optional<TilePointer> ignored = {}
  ) const;

  [[nodiscard]] bool tile_belongs_to_player(
      TilePointer ptr, Player player, std::optional<TilePointer> ignored = {}
  ) const;

  [[nodiscard]] bool neighbors_only_players(
      TilePointer ptr, Player player, std::optional<TilePointer> ignored = {}
  ) const;

  [[nodiscard]] std::unordered_set<TilePointer>
  tiles_around_hive(std::optional<TilePointer> ignored = {}) const;

  bool can_player_move(Player player, TilePointer ptr) const;

  bool can_player_place(Player player, PieceKind kind) const;

  bool can_player_place_at(Player player, TilePointer ptr) const;

  [[nodiscard]] std::generator<TilePointer>
  valid_placements(Player player) const;

  [[nodiscard]] bool moving_breaks_hive(TilePointer ptr) const;

  [[nodiscard]] std::generator<std::pair<TilePointer, Piece>>
  moveable_pieces_for(Player player) const;

  [[nodiscard]] bool
  has_neighbor(TilePointer cell, std::optional<TilePointer> ignored = {}) const;

  [[nodiscard]] bool has_neighbor_in_direction(
      TilePointer cell,
      Direction direction,
      std::optional<TilePointer> ignored = {}
  ) const;

  [[nodiscard]] bool can_move_to(
      TilePointer from,
      TilePointer to,
      bool can_leave,
      std::optional<TilePointer> ignored = {}
  ) const;

  [[nodiscard]] std::generator<TilePointer> valid_steps(
      TilePointer ptr,
      std::optional<TilePointer> ignored = {},
      bool can_leave = false
  ) const;

  [[nodiscard]] std::generator<Move> queens_moves(TilePointer queen) const;

  [[nodiscard]] std::generator<Move> beetle_moves(TilePointer beetle) const;

  [[nodiscard]] std::generator<Move>
  grasshopper_moves(TilePointer grasshopper) const;

  [[nodiscard]] std::generator<Move> spider_moves(TilePointer spider) const;

  [[nodiscard]] std::generator<Move> ant_moves(TilePointer ant) const;

  void apply_move(Move move, Player player);

  const std::map<Player, PlayerPiecesMap> &get_player_pieces() const;

private:
  std::unordered_map<TilePointer, std::vector<Piece>> data;

  static const PlayerPiecesMap DEFAULT_PLAYER_PIECES;

  std::map<Player, PlayerPiecesMap> player_pieces{
      {Player::White, DEFAULT_PLAYER_PIECES},
      {Player::Black, DEFAULT_PLAYER_PIECES},
  };

  friend std::formatter<Board>;
};

} // namespace hive

template <> struct std::formatter<hive::Board> {
  static constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin();
  }

  static auto format(const hive::Board &obj, std::format_context &ctx) {
    return std::format_to(ctx.out(), "{}", obj.data);
  }
};
