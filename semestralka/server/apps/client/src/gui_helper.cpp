#include "gui_helper.h"
#include "consts.h"
#include <algorithm>

const static std::map<hive::PieceKind, std::string_view> PIECE_LETTERS = {
    {hive::PieceKind::Queen, "Q"},
    {hive::PieceKind::Spider, "S"},
    {hive::PieceKind::Ant, "A"},
    {hive::PieceKind::Beetle, "B"},
    {hive::PieceKind::Grasshopper, "G"}
};

namespace gui {

raylib::Vector2
hex_to_pixel(const HiveGuiState &gui_state, hive::TilePointer ptr) {
  const auto pf = static_cast<float>(ptr.p);
  const auto qf = static_cast<float>(ptr.q);
  const raylib::Vector2 pos{
      (HEX_X_FACTOR * pf), (HEX_Y_FACTOR * (qf + pf / 2.0F))
  };
  return gui_state.window_center() + pos * HEX_SIZE;
}

std::string_view kind_letter(hive::PieceKind kind) {
  return PIECE_LETTERS.at(kind);
}

std::string piece_letter(const hive::Piece &piece) {
  std::string base(kind_letter(piece.kind));

  if (piece.owner == hive::Player::Black) {
    std::ranges::transform(base, base.begin(), ::tolower);
  }

  return base;
}

} // namespace gui
