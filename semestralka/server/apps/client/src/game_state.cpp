#include "game_state.h"

GameState GameState::create_default() {
  GameState state;
  state.board = hive::Board{};
  state._current_player = hive::Player::White;
  return state;
}

void GameState::switch_player() {
  _current_player = (_current_player == hive::Player::White)
                        ? hive::Player::Black
                        : hive::Player::White;
}

bool GameState::can_place_piece(
    hive::PieceKind kind, hive::TilePointer ptr
) const {
  return board.can_player_place(_current_player, kind) &&
         board.can_player_place_at(_current_player, ptr);
}

void GameState::place_piece(hive::TilePointer ptr, hive::PieceKind kind) {
  board.apply_move(hive::make_placement(ptr, kind), _current_player);
  switch_player();
}

void GameState::apply_move(const hive::Move &move) {
  board.apply_move(move, _current_player);
  switch_player();
}
