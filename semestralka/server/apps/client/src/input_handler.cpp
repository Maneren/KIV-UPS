#include "input_handler.h"
#include "consts.h"
#include "gui_helper.h"
#include <Mouse.hpp>
#include <raygui.h>
#include <raylib.h>

namespace {

std::generator<hive::TilePointer> clicable_tiles(const GameState &game) {
  for (const auto [ptr, piece] : game.board.pieces()) {
    co_yield ptr;
  }
  for (const auto ptr : game.board.tiles_around_hive()) {
    co_yield ptr;
  }
}

bool handle_kind_selection(GameState &game, HiveGuiState &gui_state) {
  const auto &available =
      game.board.get_player_pieces().at(game.current_player);

  // --- Build filtered list of available pieces for dropdown ---
  std::vector<hive::PieceKind> availableKinds;
  std::string availableString;
  availableString.reserve(available.size() * 2);

  // Single pass: filter and build string simultaneously
  for (const auto [piece_kind, count] : available) {
    if (count > 0) {
      if (!availableKinds.empty()) {
        availableString += ';';
      }
      availableKinds.push_back(piece_kind);
      availableString +=
          gui::piece_letter({.kind = piece_kind, .owner = game.current_player});
    }
  }

  const Rectangle kind_selection_box = {20, 200, 100, TEXT_FONT_SIZE + 8};

  if (!availableKinds.empty()) {
    static int dropdownIdx = 0;
    static bool dropdownActive = false;

    const auto clicked = GuiDropdownBox(
                             kind_selection_box,
                             availableString.c_str(),
                             &dropdownIdx,
                             dropdownActive
                         ) != 0;

    gui_state.select_kind(availableKinds[dropdownIdx]);

    if (clicked) {
      dropdownActive = !dropdownActive;
      return true;
    }
  } else {
    GuiLabel(kind_selection_box, "No available pieces");
  }

  return false;
}

bool tile_clicked(const HiveGuiState &gui_state, hive::TilePointer ptr) {
  const auto pos = gui::hex_to_pixel(gui_state, ptr);

  return CheckCollisionPointCircle(raylib::Mouse::GetPosition(), pos, HEX_SIZE);
}

} // namespace

void handle_input(GameState &game, HiveGuiState &gui_state) {
  if (handle_kind_selection(game, gui_state)) {
    return;
  }

  if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    return;
  }

  std::optional<hive::TilePointer> clicked;

  for (const auto ptr : clicable_tiles(game)) {
    if (tile_clicked(gui_state, ptr)) {
      clicked = ptr;
      break;
    }
  }

  if (!clicked) {
    return;
  }

  const auto ptr = *clicked;

  if (gui_state.valid_moves()) {
    const auto move =
        std::ranges::find_if(*gui_state.valid_moves(), [ptr](auto move) {
          return move.to == ptr;
        });

    if (move != gui_state.valid_moves()->end()) {
      game.apply_move(*move);
      gui_state.clear_selection();
      return;
    }
  }

  if (game.board.is_empty(ptr)) {
    const auto kind = gui_state.selected_kind();

    if (game.can_place_piece(kind, ptr)) {
      game.place_piece(ptr, kind);
    }

    return;
  }

  const auto piece = game.board.get(ptr).back();

  if (piece.owner == game.current_player &&
      !game.board.moving_breaks_hive(ptr)) {
    gui_state.update_valid_moves(game.board, ptr, piece);
    return;
  }
}
