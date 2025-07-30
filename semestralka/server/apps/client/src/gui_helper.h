#include "gui_state.h"
#include <Vector2.hpp>
#include <hive/types.h>

namespace gui {

[[nodiscard]] raylib::Vector2
hex_to_pixel(const HiveGuiState &gui_state, hive::TilePointer ptr);

std::string_view kind_letter(hive::PieceKind kind);

std::string piece_letter(const hive::Piece &piece);

} // namespace gui
