#include "game_state.h"
#include "gui_state.h"
#include "input_handler.h"
#include "renderer.h"
#include <Color.hpp>
#include <Window.hpp>
#include <hive/board.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

using namespace hive;

int main() {
  constexpr int DEFAUTL_WINDOW_WIDTH = 800;
  constexpr int DEFAULT_WINDOW_HEIGHT = 600;
  constexpr int TARGET_FPS = 60;

  auto game = GameState::create_default();
  raylib::Window window{
      DEFAUTL_WINDOW_WIDTH,
      DEFAULT_WINDOW_HEIGHT,
      "Hive Game - Raylib Client",
      FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE
  };
  window.SetTargetFPS(TARGET_FPS);

  HiveGuiState gui;
  gui.resize(raylib::Window::GetSize());

  while (!raylib::Window::ShouldClose()) {
    if (raylib::Window::IsResized()) {
      gui.resize(raylib::Window::GetSize());
    }

    window.BeginDrawing();
    window.ClearBackground(raylib::Color::RayWhite());

    handle_input(game, gui);

    renderer::draw(game, gui);

    window.EndDrawing();
  }

  return 0;
}
