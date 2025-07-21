#include "game_state.h"

GameState GameState::create_default() {
  GameState state;
  state.board = hive::Board{};
  state.current_player = hive::Player::White;
  state.available_white = {
      {hive::PieceKind::Ant, 2},
      {hive::PieceKind::Grasshopper, 2},
      {hive::PieceKind::Beetle, 2},
      {hive::PieceKind::Spider, 2},
      {hive::PieceKind::Queen, 1}
  };
  state.available_black = state.available_white;
  return state;
}

void GameState::switch_player() {
  current_player = (current_player == hive::Player::White)
                       ? hive::Player::Black
                       : hive::Player::White;
}

bool GameState::can_place_piece(hive::PieceKind kind) const {
  const auto &available =
      current_player == hive::Player::White ? available_white : available_black;
  return available.at(kind) > 0;
}

void GameState::place_piece(hive::TilePointer pos, hive::PieceKind kind) {
  if (!can_place_piece(kind)) {
    return;
  }

  board.add_piece(pos, hive::Piece{.kind = kind, .owner = current_player});
  auto &available =
      current_player == hive::Player::White ? available_white : available_black;
  available.at(kind)--;
  switch_player();
}
