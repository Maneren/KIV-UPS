#pragma once

#include <Vector2.hpp>
#include <hive/board.h>
#include <vector>

struct HiveGuiState {
  [[nodiscard]] raylib::Vector2 window_size() const { return _window_size; }
  [[nodiscard]] raylib::Vector2 window_center() const { return _window_center; }

  [[nodiscard]] std::optional<hive::TilePointer> selected_tile() const {
    return _selected_tile;
  }

  [[nodiscard]] const std::optional<std::vector<hive::Move>> &
  valid_moves() const {
    return _valid_moves;
  }
  std::optional<std::vector<hive::Move>> &valid_moves_mut() {
    return _valid_moves;
  }
  [[nodiscard]] hive::PieceKind selected_kind() const { return _selected_kind; }
  void select_kind(hive::PieceKind kind) { _selected_kind = kind; }

  void select_tile(hive::TilePointer tile);

  void clear_selection();
  void update_valid_moves(
      hive::Board &board, hive::TilePointer tile, hive::Piece piece
  );

  void resize(const raylib::Vector2 &new_window_size);

private:
  raylib::Vector2 _window_size;
  raylib::Vector2 _window_center;

  std::optional<hive::TilePointer> _selected_tile = std::nullopt;
  std::optional<std::vector<hive::Move>> _valid_moves;
  hive::PieceKind _selected_kind = hive::PieceKind::Queen;
};
