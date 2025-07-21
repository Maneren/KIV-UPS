#include "input_handler.h"
#include <raylib.h>

void handle_input(GameState &game, HiveGuiState &gui) {
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    Vector2 mouse = GetMousePosition();
    hive::TilePointer clicked{-999, -999};
    for (auto [ptr, piece] : game.board.pieces()) {
      Vector2 pos = {
          400 + 40 * (3.0F / 2.0F * ptr.p),
          300 + 40 * (std::numbers::sqrt3_v<float> * (ptr.q + ptr.p / 2.0F))
      };
      if (CheckCollisionPointCircle(mouse, pos, 40)) {
        clicked = ptr;
        break;
      }
    }
    if (clicked.p == -999) {
      for (auto ptr : game.board.tiles_around_hive()) {
        Vector2 pos = {
            400 + 40 * (3.0F / 2.0F * ptr.p),
            300 + 40 * (std::numbers::sqrt3_v<float> * (ptr.q + ptr.p / 2.0F))
        };
        if (CheckCollisionPointCircle(mouse, pos, 40)) {
          clicked = ptr;
          break;
        }
      }
    }
    if (clicked.p != -999) {
      if (gui.selecting_move) {
        for (const auto &move : gui.valid_moves) {
          if (move.to == clicked) {
            game.board.add_piece(move.to, game.board.remove_piece(move.from));
            gui.clear_selection();
            game.switch_player();
            break;
          }
        }
      } else {
        if (!game.board.is_empty(clicked)) {
          const auto &pieces = game.board.get(clicked);
          const auto &piece = pieces.back();
          if (piece.owner == game.current_player &&
              !game.board.moving_breaks_hive(clicked)) {
            gui.valid_moves.clear();
            for (auto move : game.board.moves_for_piece(clicked, piece)) {
              gui.valid_moves.push_back(move);
            }
            if (!gui.valid_moves.empty()) {
              gui.selecting_move = true;
              gui.selected_tile = clicked;
            }
          }
        } else {
          // --- Place selected piece kind if valid ---
          if (game.can_place_piece(gui.selected_kind) &&
              game.board.can_player_place_at(game.current_player, clicked)) {
            game.place_piece(clicked, gui.selected_kind);
          }
        }
      }
    }
  }
}
