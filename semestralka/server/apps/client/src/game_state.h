#pragma once
#include <hive/board.h>

class GameState {
public:
  hive::Board board{};
  hive::Player current_player{};

  static GameState create_default();

  void switch_player();
  bool can_place_piece(hive::PieceKind kind) const;
  void place_piece(hive::TilePointer pos, hive::PieceKind kind);
};
