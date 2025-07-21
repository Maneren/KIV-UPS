#pragma once
#include <hive/board.h>

class GameState {
public:
  hive::Board board{};
  hive::Player current_player{};
  hive::PlayerPiecesMap available_white;
  hive::PlayerPiecesMap available_black;

  static GameState create_default();

  void switch_player();
  bool can_place_piece(hive::PieceKind kind) const;
  void place_piece(hive::TilePointer pos, hive::PieceKind kind);
};
