#pragma once

#include <Vector2.hpp>
#include <hive/board.h>
#include <vector>

struct HiveGuiState {
  raylib::Vector2 window_size;
  std::optional<hive::TilePointer> selected_tile = std::nullopt;
  bool selecting_move = false;
  std::vector<hive::Move> valid_moves;
  hive::PieceKind selected_kind = hive::PieceKind::Queen;

  void select_tile(hive::TilePointer tile);
  void clear_selection();
  void update_valid_moves(
      hive::Board &board, hive::TilePointer tile, hive::PieceKind kind
  );
};
