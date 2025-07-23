#include "board_renderer.h"
#include "game_state.h"
#include "gui_state.h"
#include "input_handler.h"
#include <Color.hpp>
#include <Window.hpp>
#include <hive/board.h>

using namespace hive;

int main() {
  GameState game = GameState::create_default();
  HiveGuiState gui;
  // Initial board setup
  game.board.apply_move(
      make_placement({.p = 0, .q = 0}, PieceKind::Queen), Player::Black
  );
  game.board.apply_move(
      make_placement({.p = 1, .q = 0}, PieceKind::Queen), Player::White
  );
  game.board.apply_move(
      make_placement({.p = 0, .q = 1}, PieceKind::Spider), Player::Black
  );
  game.board.apply_move(
      make_placement({.p = 1, .q = 1}, PieceKind::Ant), Player::White
  );

  raylib::Window window{
      800, 600, "Hive Game - Raylib Client", FLAG_MSAA_4X_HINT
  };
  window.SetTargetFPS(60);

  gui.window_size = raylib::Vector2{800, 600};

  while (!raylib::Window::ShouldClose()) {
    window.BeginDrawing();
    window.ClearBackground(raylib::Color::RayWhite());

    BoardRenderer::draw(game, gui);
    BoardRenderer::draw_available(game, gui);

    handle_input(game, gui);

    window.EndDrawing();
  }

  return 0;
}
