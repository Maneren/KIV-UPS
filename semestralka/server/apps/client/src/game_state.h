#pragma once
#include <hive/board.h>

class GameState {
public:
  hive::Board board{};
  hive::Player current_player{};

  static GameState create_default();

  void switch_player();
  bool can_place_piece(hive::PieceKind kind, hive::TilePointer ptr) const;
  void place_piece(hive::TilePointer ptr, hive::PieceKind kind);

  void apply_move(const hive::Move &move);
};
