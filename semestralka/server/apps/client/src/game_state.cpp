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

bool GameState::can_place_piece(hive::PieceKind kind) const {
  return board.can_player_place(current_player, kind);
}

void GameState::place_piece(hive::TilePointer pos, hive::PieceKind kind) {
  if (!can_place_piece(kind)) {
    return;
  }

  board.apply_move(hive::make_placement(pos, kind), current_player);
  switch_player();
}
