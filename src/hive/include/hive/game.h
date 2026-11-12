#pragma once

#include <cstdint>
#include <hive/board.h>
#include <hive/types.h>
#include <optional>
#include <vector>

namespace hive {

enum class GameResult : std::uint8_t {
  Ongoing = 0,
  WhiteWins = 1,
  BlackWins = 2,
  Draw = 3,
};

class Game {
public:
  Game();
  explicit Game(Board board, Player current = Player::White);

  [[nodiscard]] const Board &board() const noexcept;
  [[nodiscard]] Player current_player() const noexcept;
  [[nodiscard]] GameResult result() const noexcept;
  [[nodiscard]] bool is_over() const noexcept;
  [[nodiscard]] std::optional<Player> winner() const noexcept;

  [[nodiscard]] std::vector<Move> legal_moves() const;
  [[nodiscard]] bool has_legal_move() const;
  [[nodiscard]] bool can_place(PieceKind kind) const;
  [[nodiscard]] bool can_place_at(TilePointer ptr) const;
  [[nodiscard]] std::vector<TilePointer> placement_targets() const;
  [[nodiscard]] bool can_move_from(TilePointer ptr) const;
  [[nodiscard]] std::vector<Move> moves_from(TilePointer ptr) const;
  [[nodiscard]] bool is_legal(const Move &move) const;

  bool submit(const Move &move);
  void apply(const Move &move);
  bool pass_if_stuck();

  [[nodiscard]] std::optional<TilePointer> queen_position(Player player) const;

private:
  void check_win();

  Board _board;
  Player _current_player{Player::White};
  GameResult _result{GameResult::Ongoing};
};

} // namespace hive
