#include <hive/types.h>

namespace hive {

Move make_move(TilePointer from, TilePointer to, PieceKind piece_kind) {
  return Move{MovePiece{.from = from, .to = to, .piece_kind = piece_kind}};
}

Move make_placement(TilePointer pos, PieceKind piece_kind) {
  return Move{PlacePiece{.pos = pos, .piece_kind = piece_kind}};
}

} // namespace hive
