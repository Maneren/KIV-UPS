#include <hive/game.h>

#include <algorithm>
#include <cstdint>
#include <hive/board.h>
#include <hive/types.h>
#include <optional>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace hive {

namespace {

Player other(Player player) {
  return player == Player::White ? Player::Black : Player::White;
}

auto move_key(const Move &move) {
  return std::visit(
      [](const auto &value) {
        using T = std::decay_t<decltype(value)>;
        const auto kind = static_cast<std::uint8_t>(value.piece_kind);

        if constexpr (std::is_same_v<T, MovePiece>) {
          return std::tuple{
              0,
              value.from.p,
              value.from.q,
              value.to.p,
              value.to.q,
              kind,
          };
        } else {
          return std::tuple{
              1,
              value.pos.p,
              value.pos.q,
              value.pos.p,
              value.pos.q,
              kind,
          };
        }
      },
      move
  );
}

void sort_moves(std::vector<Move> &moves) {
  std::ranges::sort(moves, [](const Move &lhs, const Move &rhs) {
    return move_key(lhs) < move_key(rhs);
  });
}

} // namespace

Game::Game() = default;

Game::Game(Board board, Player current)
    : _board(std::move(board)), _current_player(current) {
  check_win();
}

const Board &Game::board() const noexcept { return _board; }

Player Game::current_player() const noexcept { return _current_player; }

GameResult Game::result() const noexcept { return _result; }

bool Game::is_over() const noexcept { return _result != GameResult::Ongoing; }

std::optional<Player> Game::winner() const noexcept {
  if (_result == GameResult::WhiteWins) {
    return Player::White;
  }
  if (_result == GameResult::BlackWins) {
    return Player::Black;
  }
  return std::nullopt;
}

std::vector<Move> Game::legal_moves() const {
  auto moves = _board.moves_for_player(
                   _current_player, _board.remaining(_current_player)
               ) |
               std::ranges::to<std::vector>();
  sort_moves(moves);
  return moves;
}

bool Game::has_legal_move() const {
  return std::ranges::any_of(
      _board.moves_for_player(
          _current_player, _board.remaining(_current_player)
      ),
      [](const Move & /*move*/) { return true; }
  );
}

bool Game::can_place(PieceKind kind) const {
  return !is_over() && _board.can_player_place(_current_player, kind);
}

bool Game::can_place_at(TilePointer ptr) const {
  if (is_over()) {
    return false;
  }
  if (_board.is_empty_all()) {
    return ptr == TilePointer{.p = 0, .q = 0};
  }
  return _board.can_player_place_at(_current_player, ptr);
}

std::vector<TilePointer> Game::placement_targets() const {
  if (is_over()) {
    return {};
  }
  if (_board.is_empty_all()) {
    const auto remaining = _board.remaining(_current_player);
    const bool has_pieces =
        std::ranges::any_of(remaining, [](const auto &entry) {
          return entry.second > 0;
        });
    if (!has_pieces) {
      return {};
    }
    return {TilePointer{.p = 0, .q = 0}};
  }

  auto targets =
      _board.valid_placements(_current_player) | std::ranges::to<std::vector>();
  std::ranges::sort(targets, [](TilePointer lhs, TilePointer rhs) {
    return std::pair{lhs.p, lhs.q} < std::pair{rhs.p, rhs.q};
  });
  return targets;
}

bool Game::can_move_from(TilePointer ptr) const {
  return !is_over() && !_board.is_empty(ptr) &&
         _board.get_top(ptr).owner == _current_player &&
         _board.can_player_move(_current_player, ptr);
}

std::vector<Move> Game::moves_from(TilePointer ptr) const {
  if (!can_move_from(ptr)) {
    return {};
  }

  auto moves = _board.moves_for_piece(ptr, _board.get_top(ptr)) |
               std::ranges::to<std::vector>();
  sort_moves(moves);
  return moves;
}

bool Game::is_legal(const Move &move) const {
  if (is_over()) {
    return false;
  }

  if (const auto *placement = std::get_if<PlacePiece>(&move)) {
    return can_place(placement->piece_kind) && can_place_at(placement->pos);
  }

  const auto &movement = std::get<MovePiece>(move);
  const auto candidates = moves_from(movement.from);
  return std::ranges::find(candidates, move) != candidates.end();
}

bool Game::submit(const Move &move) {
  if (!is_legal(move)) {
    return false;
  }
  apply(move);
  return true;
}

void Game::apply(const Move &move) {
  _board.apply_move(move, _current_player);
  _current_player = other(_current_player);
  check_win();
}

bool Game::pass_if_stuck() {
  if (is_over() || _board.is_empty_all() || has_legal_move()) {
    return false;
  }
  _current_player = other(_current_player);
  return true;
}

std::optional<TilePointer> Game::queen_position(Player player) const {
  for (const auto &[ptr, piece] : _board.pieces()) {
    if (piece.owner == player && piece.kind == PieceKind::Queen) {
      return ptr;
    }
  }
  return std::nullopt;
}

void Game::check_win() {
  for (const auto player : {Player::White, Player::Black}) {
    const auto queen_pos = queen_position(player);
    if (!queen_pos.has_value()) {
      continue;
    }
    if (_board.surrounding_count(*queen_pos) == DIRECTIONS.size()) {
      _result = player == Player::White ? GameResult::BlackWins
                                        : GameResult::WhiteWins;
      return;
    }
  }
}

} // namespace hive
