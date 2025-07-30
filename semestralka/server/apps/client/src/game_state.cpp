#include "game_state.h"

GameState GameState::create_default() {
  GameState state;
  state.board = hive::Board{};
  state.current_player = hive::Player::White;
  return state;
}

void GameState::switch_player() {
  current_player = (current_player == hive::Player::White)
                       ? hive::Player::Black
                       : hive::Player::White;
}

bool GameState::can_place_piece(
    hive::PieceKind kind, hive::TilePointer ptr
) const {
  return board.can_player_place(current_player, kind) &&
         board.can_player_place_at(current_player, ptr);
}

void GameState::place_piece(hive::TilePointer ptr, hive::PieceKind kind) {
  board.apply_move(hive::make_placement(ptr, kind), current_player);
  switch_player();
}

void GameState::apply_move(const hive::Move &move) {
  board.apply_move(move, current_player);
  switch_player();
}
