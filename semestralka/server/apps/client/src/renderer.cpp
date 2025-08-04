#include "renderer.h"
#include "consts.h"
#include "gui_helper.h"
#include "rayutils/text.h"
#include <Functions.hpp>
#include <Rectangle.hpp>
#include <Text.hpp>
#include <map>
#include <raygui.h>
#include <raylib-cpp.hpp>
#include <rayutils/format.h>
#include <utils/print.h>

constexpr float OUTLINE_THICKNESS = 2.0F;
constexpr float OUTLINE_THICKNESS_SPECIAL = 3.0F;
constexpr float AVAILABLE_Y_OFFSET = 20.F;
constexpr float AVAILABLE_X_OFFSET = 20.F;
constexpr float AVAILABLE_TEXT_FONT_SIZE = 24.F;

namespace {

const raylib::Font font;

void draw_hex_outline(
    const raylib::Vector2 &center,
    const raylib::Color &color,
    float thickness = OUTLINE_THICKNESS
) {
  for (int i = 0; i < HEX_SIDES; ++i) {
    const float start_angle = static_cast<float>(i) * HEX_ANGLE_STEP;
    const float end_angle =
        static_cast<float>((i + 1) % HEX_SIDES) * HEX_ANGLE_STEP;

    const raylib::Vector2 start{
        center.x + (HEX_SIZE * cosf(start_angle)),
        center.y + (HEX_SIZE * sinf(start_angle))
    };
    const raylib::Vector2 end{
        center.x + (HEX_SIZE * cosf(end_angle)),
        center.y + (HEX_SIZE * sinf(end_angle))
    };

    DrawLineEx(start, end, thickness, color);
  }
}

const std::map<hive::PieceKind, raylib::Color> PIECE_COLORS = {
    {hive::PieceKind::Queen, raylib::Color{205, 180, 0, 255}},
    {hive::PieceKind::Spider, raylib::Color::Red()},
    {hive::PieceKind::Ant, raylib::Color::Blue()},
    {hive::PieceKind::Beetle, raylib::Color::Purple()},
    {hive::PieceKind::Grasshopper, raylib::Color::DarkGreen()}
};

void draw_turn_label(const GameState &game, const HiveGuiState &gui) {
  const std::string turnText = game.current_player == hive::Player::White
                                   ? "White's turn"
                                   : "Black's turn";
  const auto label_width =
      static_cast<float>(raylib::MeasureText(turnText.data(), TEXT_FONT_SIZE));

  constexpr float label_height = 24.F;
  const float label_x = (gui.window_center().x) - (label_width / 2.F);
  constexpr float label_y = 10.F;

  GuiLabel(
      raylib::Rectangle{label_x, label_y, label_width, label_height},
      turnText.c_str()
  );
}

void draw_tiles(const GameState &game, const HiveGuiState &gui_state) {
  for (const auto &[ptr, piece] : game.board.pieces()) {
    const raylib::Vector2 pos = gui::hex_to_pixel(gui_state, ptr);

    // Draw hexagon
    const auto tile_color = PIECE_COLORS.at(piece.kind);
    pos.DrawPoly(HEX_SIDES, HEX_SIZE, 0, tile_color);

    const auto text_color = piece.owner == hive::Player::White
                                ? raylib::Color::White()
                                : raylib::Color::Black();

    const raylib::Text letter =
        rayutils::text(gui::piece_letter(piece), TEXT_FONT_SIZE, text_color);

    const auto letter_dimensions = letter.MeasureEx();
    letter.Draw(pos - letter_dimensions / 2.F);

    draw_hex_outline(pos, raylib::Color::DarkGray());
  }

  // Highlight valid moves
  if (gui_state.valid_moves()) {
    for (const auto &move : *gui_state.valid_moves()) {
      const auto pos = gui::hex_to_pixel(gui_state, move.to);
      draw_hex_outline(pos, raylib::Color::Green(), OUTLINE_THICKNESS_SPECIAL);
    }
  }

  // Highlight selected tile
  if (gui_state.selected_tile()) {
    draw_hex_outline(
        gui::hex_to_pixel(gui_state, *gui_state.selected_tile()),
        raylib::Color::Black(),
        OUTLINE_THICKNESS_SPECIAL
    );
  }
}

void draw_available(const GameState &game) {
  const auto &available =
      game.board.get_player_pieces().at(game.current_player);

  auto text = rayutils::empty_text(
      AVAILABLE_TEXT_FONT_SIZE, raylib::Color::DarkGray(), font
  );

  for (const auto [piece_kind, count] : available) {
    if (!text.text.empty()) {
      text.text += '\n';
    }

    text.text += std::format("{}: {}", gui::kind_letter(piece_kind), count);
  }

  text.Draw(AVAILABLE_X_OFFSET, AVAILABLE_Y_OFFSET);
}

} // namespace

namespace renderer {

void draw(const GameState &game, const HiveGuiState &gui_state) {
  GuiSetStyle(0, TEXT_SIZE, TEXT_FONT_SIZE);

  draw_turn_label(game, gui_state);

  draw_tiles(game, gui_state);

  draw_available(game);
}

} // namespace renderer
