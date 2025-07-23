#pragma once

#include "game_state.h"
#include "gui_state.h"

class BoardRenderer {
public:
  static void draw(const GameState &game, const HiveGuiState &gui);
  static void draw_available(const GameState &game, HiveGuiState &gui);
};
