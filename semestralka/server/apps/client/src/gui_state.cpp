#include "gui_state.h"
#include <optional>

void HiveGuiState::select_tile(hive::TilePointer tile) {
  _selected_tile = std::make_optional(tile);
}

void HiveGuiState::unselect_tile() { _selected_tile = std::nullopt; }

void HiveGuiState::clear_selection() {
  _selected_tile = std::nullopt;
  _valid_moves = std::nullopt;
}

void HiveGuiState::update_valid_moves(
    hive::Board &board, hive::TilePointer tile, hive::Piece piece
) {
  if (board.is_empty(tile)) {
    return;
  }

  _valid_moves = std::make_optional<std::vector<hive::Move>>();
  for (auto move : board.moves_for_piece(tile, piece)) {
    _valid_moves->push_back(move);
  }
}

void HiveGuiState::resize(const raylib::Vector2 &new_window_size) {
  _window_size = new_window_size;
  _window_center = _window_size * 0.5F;
}
