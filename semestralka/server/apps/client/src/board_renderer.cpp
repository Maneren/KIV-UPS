#include "board_renderer.h"
#include "Functions.hpp"
#include <algorithm>
#include <map>
#include <numbers>
#include <raylib-cpp.hpp>
#include <utils/print.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

constexpr float HEX_SIZE = 40.0F;
constexpr int BOARD_OFFSET_X = 400;
constexpr int BOARD_OFFSET_Y = 300;
const static std::map<hive::PieceKind, std::string_view> PIECE_LETTERS = {
    {hive::PieceKind::Queen, "Q"},
    {hive::PieceKind::Spider, "S"},
    {hive::PieceKind::Ant, "A"},
    {hive::PieceKind::Beetle, "B"},
    {hive::PieceKind::Grasshopper, "G"}
};

constexpr float HEX_X_FACTOR = 3.0F / 2.0F;
constexpr float HEX_Y_FACTOR = std::numbers::sqrt3_v<float>;
constexpr float OUTLINE_THICKNESS = 3.0F;
constexpr float HEX_ANGLE_STEP = 60.0F * (std::numbers::pi_v<float> / 180.0F);
constexpr int HEX_SIDES = 6;
constexpr int TEXT_FONT_SIZE = 32;
constexpr int TEXT_FONT_SPACING = 5;
constexpr float TEXT_OFFSET = 10.F;
constexpr int AVAILABLE_Y_OFFSET = 20;
constexpr int AVAILABLE_TEXT_X_OFFSET = 20;
constexpr int AVAILABLE_TEXT_Y_STEP = 20;
constexpr int AVAILABLE_TEXT_FONT_SIZE = 16;

namespace {

const raylib::Font font;

raylib::Vector2 hex_to_pixel(hive::TilePointer ptr) {
  const auto pf = static_cast<float>(ptr.p);
  const auto qf = static_cast<float>(ptr.q);
  const float x = HEX_SIZE * (HEX_X_FACTOR * pf);
  const float y = HEX_SIZE * (HEX_Y_FACTOR * (qf + pf / 2.0F));
  return {
      static_cast<float>(BOARD_OFFSET_X) + x,
      static_cast<float>(BOARD_OFFSET_Y) + y
  };
}

std::string piece_letter(const hive::Piece &piece) {
  std::string base(PIECE_LETTERS.at(piece.kind));

  if (piece.owner == hive::Player::Black) {
    std::ranges::transform(base, base.begin(), ::tolower);
  }

  return base;
}

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

} // namespace

void BoardRenderer::draw(const GameState &game, const HiveGuiState &gui) {
  GuiSetStyle(0, TEXT_SIZE, TEXT_FONT_SIZE);

  // --- Draw current player's turn label centered at top ---
  const std::string turnText = game.current_player == hive::Player::White
                                   ? "White's turn"
                                   : "Black's turn";
  const auto label_width =
      static_cast<float>(raylib::MeasureText(turnText.data(), TEXT_FONT_SIZE));

  constexpr float label_height = 24.F;
  const float label_x = (gui.window_size.x / 2.F) - (label_width / 2.F);
  constexpr float label_y = 10.F;

  GuiLabel(
      Rectangle{label_x, label_y, label_width, label_height}, turnText.data()
  );

  for (const auto &[ptr, piece] : game.board.pieces()) {
    const raylib::Vector2 pos = hex_to_pixel(ptr);

    // Draw hexagon
    pos.DrawPoly(HEX_SIDES, HEX_SIZE, 0, raylib::Color::LightGray());

    const std::string &letter = piece_letter(piece);
    const raylib::Vector2 textPos(pos.x - TEXT_OFFSET, pos.y - TEXT_OFFSET);
    font.DrawText(
        letter,
        textPos,
        TEXT_FONT_SIZE,
        TEXT_FONT_SPACING,
        raylib::Color::Black()
    );

    draw_hex_outline(pos, raylib::Color::DarkGray(), 2.0F);

    // Draw selection indicator if this tile is selected
    if (ptr == gui.selected_tile) {
      draw_hex_outline(pos, raylib::Color::Blue());
    }
  }
}

void BoardRenderer::draw_available(const GameState &game, HiveGuiState &gui) {
  const auto &available = game.current_player == hive::Player::White
                              ? game.available_white
                              : game.available_black;

  // Cache color and font outside the loop
  const raylib::Color textColor(
      game.current_player == hive::Player::White ? BLACK : DARKGRAY
  );

  // Pre-calculate base position
  const auto baseX = static_cast<float>(AVAILABLE_TEXT_X_OFFSET);
  const auto baseY = static_cast<float>(AVAILABLE_Y_OFFSET);
  const auto yStep = static_cast<float>(AVAILABLE_TEXT_Y_STEP);

  for (auto i = 0U; const auto [piece_kind, count] : available) {
    const std::string txt =
        std::format("{}: {}", PIECE_LETTERS.at(piece_kind), count);

    font.DrawText(
        txt,
        {baseX, baseY + (static_cast<float>(i++) * yStep)},
        AVAILABLE_TEXT_FONT_SIZE,
        TEXT_FONT_SPACING,
        textColor
    );
  }

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
          piece_letter({.kind = piece_kind, .owner = game.current_player});
    }
  }

  // --- Only show dropdown if there are available pieces ---
  if (!availableKinds.empty()) {
    constexpr float DROPDOWN_WIDTH = 100;
    constexpr float DROPDOWN_HEIGHT = TEXT_FONT_SIZE + 8;
    constexpr float DROPDOWN_X = 20;
    constexpr float DROPDOWN_Y = 200;

    // --- Draw dropdown for piece selection ---
    static int dropdownIdx = 0;
    static bool dropdownActive = false;
    if (GuiDropdownBox(
            {DROPDOWN_X, DROPDOWN_Y, DROPDOWN_WIDTH, DROPDOWN_HEIGHT},
            availableString.c_str(),
            &dropdownIdx,
            dropdownActive
        ) != 0) {
      dropdownActive = !dropdownActive;
    }

    // --- Update selected_kind in gui if changed ---
    gui.selected_kind =
        static_cast<hive::PieceKind>(availableKinds[dropdownIdx]);
  }
}

void BoardRenderer::draw_valid_moves(const HiveGuiState &gui) {
  for (const auto &move : gui.valid_moves) {
    const auto pos = hex_to_pixel(move.to);
    draw_hex_outline(pos, raylib::Color::Green());
  }
}
