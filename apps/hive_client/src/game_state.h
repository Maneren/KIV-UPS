#pragma once

#include <hive/board.h>

class GameState {
  hive::Player _current_player{};

public:
  hive::Board board{};

  static GameState create_default();

  hive::Player current_player() const { return _current_player; }

  void switch_player();
  bool can_place_piece(hive::PieceKind kind, hive::TilePointer ptr) const;
  void place_piece(hive::TilePointer ptr, hive::PieceKind kind);

  void apply_move(const hive::Move &move);
};
