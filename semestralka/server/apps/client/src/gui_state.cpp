#include "gui_state.h"

void HiveGuiState::select_tile(hive::TilePointer tile) {
  selected_tile = std::make_optional(tile);
}

void HiveGuiState::clear_selection() {
  selected_tile = std::nullopt;
  selecting_move = false;
  valid_moves.clear();
}

void HiveGuiState::update_valid_moves(
    hive::Board &board, hive::TilePointer tile, hive::PieceKind kind
) {
  valid_moves.clear();
  if (!board.is_empty(tile)) {
    const auto &pieces = board.get(tile);
    const auto &piece = pieces.back();
    if (piece.kind == kind) {
      for (auto move : board.moves_for_piece(tile, piece)) {
        valid_moves.push_back(move);
      }
    }
  }
}
