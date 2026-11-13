#include <algorithm>
#include <cstddef>
#include <format>
#include <hive/errors.h>
#include <hive/game.h>
#include <hive/types.h>
#include <limits>
#include <nanobind/nanobind.h>
#include <optional>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace nb = nanobind;

namespace {

using hive::Coordinate;
using hive::Game;
using hive::Move;
using hive::Piece;
using hive::PieceKind;
using hive::Player;
using hive::TilePointer;

constexpr std::size_t MOVE_RECORD_SIZE = 6;
constexpr int PLACE = 0;
constexpr int MOVE = 1;

int read_int(const nb::tuple &record, std::size_t index, const char *what) {
  try {
    return nb::cast<int>(record[index]);
  } catch (const nb::cast_error &) {
    throw nb::value_error(std::format("{} must be an integer", what).c_str());
  }
}

Coordinate
read_coordinate(const nb::tuple &record, std::size_t index, const char *what) {
  long long value = 0;
  try {
    value = nb::cast<long long>(record[index]);
  } catch (const nb::cast_error &) {
    throw nb::value_error(std::format("{} must be an integer", what).c_str());
  }

  if (value < std::numeric_limits<Coordinate>::min() ||
      value > std::numeric_limits<Coordinate>::max()) {
    throw nb::value_error(std::format("{} is out of range", what).c_str());
  }

  return static_cast<Coordinate>(value);
}

void check_range(int value, int min, int max, const char *what) {
  if (value < min || value > max) {
    throw nb::value_error(
        std::format("{} must be between {} and {}", what, min, max).c_str()
    );
  }
}

Player read_player(int value) {
  check_range(value, 0, 1, "player");
  return static_cast<Player>(value);
}

PieceKind read_piece_kind(int value) {
  check_range(
      value, 0, static_cast<int>(hive::NUMBER_OF_PIECES) - 1, "piece_kind"
  );
  return static_cast<PieceKind>(value);
}

TilePointer read_tile(int p, int q) {
  return {
      .p = static_cast<Coordinate>(p),
      .q = static_cast<Coordinate>(q),
  };
}

Move read_move(const nb::tuple &record) {
  if (nb::len(record) != MOVE_RECORD_SIZE) {
    throw nb::value_error("move record must have 6 elements");
  }

  const int type = read_int(record, 0, "move type");
  const auto from_p = read_coordinate(record, 1, "from_p");
  const auto from_q = read_coordinate(record, 2, "from_q");
  const auto to_p = read_coordinate(record, 3, "to_p");
  const auto to_q = read_coordinate(record, 4, "to_q");
  const auto kind = read_piece_kind(read_int(record, 5, "piece_kind"));

  const auto to = TilePointer{.p = to_p, .q = to_q};

  if (type == PLACE) {
    return hive::make_placement(to, kind);
  }
  if (type == MOVE) {
    return hive::make_move(TilePointer{.p = from_p, .q = from_q}, to, kind);
  }

  throw nb::value_error("move type must be 0 (place) or 1 (move)");
}

nb::tuple encode_tile(TilePointer ptr) { return nb::make_tuple(ptr.p, ptr.q); }

nb::tuple encode_piece(const Piece &piece) {
  return nb::make_tuple(
      static_cast<int>(piece.kind), static_cast<int>(piece.owner)
  );
}

nb::tuple encode_move(const Move &move) {
  if (const auto *placement = std::get_if<hive::PlacePiece>(&move)) {
    return nb::make_tuple(
        PLACE,
        0,
        0,
        placement->pos.p,
        placement->pos.q,
        static_cast<int>(placement->piece_kind)
    );
  }

  const auto &movement = std::get<hive::MovePiece>(move);
  return nb::make_tuple(
      MOVE,
      movement.from.p,
      movement.from.q,
      movement.to.p,
      movement.to.q,
      static_cast<int>(movement.piece_kind)
  );
}

nb::list encode_moves(const std::vector<Move> &moves) {
  nb::list result;
  for (const auto &move : moves) {
    result.append(encode_move(move));
  }
  return result;
}

nb::list encode_tiles(std::vector<TilePointer> tiles) {
  std::ranges::sort(tiles, [](TilePointer lhs, TilePointer rhs) {
    return std::pair{lhs.p, lhs.q} < std::pair{rhs.p, rhs.q};
  });

  nb::list result;
  for (const auto tile : tiles) {
    result.append(encode_tile(tile));
  }
  return result;
}

nb::list encode_tiles(const std::unordered_set<TilePointer> &tiles) {
  return encode_tiles(std::vector<TilePointer>(tiles.begin(), tiles.end()));
}

nb::object encode_optional_tile(const std::optional<TilePointer> &ptr) {
  if (!ptr.has_value()) {
    return nb::none();
  }
  return encode_tile(*ptr);
}

nb::object encode_winner(const std::optional<Player> &winner) {
  if (!winner.has_value()) {
    return nb::none();
  }
  return nb::cast(static_cast<int>(*winner));
}

} // namespace

// NOLINTNEXTLINE(performance-unnecessary-value-param)
NB_MODULE(hive_native, m) {
  const auto hive_error =
      // NOLINTNEXTLINE(misc-include-cleaner)
      nb::exception<hive::HiveError>(m, "HiveError", PyExc_RuntimeError);
  [[maybe_unused]] const auto illegal_move_error =
      // NOLINTNEXTLINE(bugprone-unused-raii, bugprone-throw-keyword-missing)
      nb::exception<hive::IllegalMoveError>(m, "IllegalMoveError", hive_error);
  [[maybe_unused]] const auto empty_tile_error =
      // NOLINTNEXTLINE(bugprone-unused-raii, bugprone-throw-keyword-missing)
      nb::exception<hive::EmptyTileError>(m, "EmptyTileError", hive_error);

  nb::class_<Game>(m, "Game")
      .def(nb::init<>())
      .def_prop_ro(
          "current_player",
          [](const Game &game) {
            return static_cast<int>(game.current_player());
          }
      )
      .def_prop_ro(
          "result",
          [](const Game &game) { return static_cast<int>(game.result()); }
      )
      .def_prop_ro(
          "winner",
          [](const Game &game) { return encode_winner(game.winner()); }
      )
      .def("is_over", &Game::is_over)
      .def(
          "is_empty_all",
          [](const Game &game) { return game.board().is_empty_all(); }
      )
      .def(
          "stacks",
          [](const Game &game) {
            nb::list result;
            for (const auto &stack : game.board().stacks()) {
              nb::list pieces;
              for (const auto &piece : stack.pieces) {
                pieces.append(encode_piece(piece));
              }
              result.append(nb::make_tuple(stack.ptr.p, stack.ptr.q, pieces));
            }
            return result;
          }
      )
      .def(
          "occupied_tiles",
          [](const Game &game) {
            return encode_tiles(game.board().occupied_tiles());
          }
      )
      .def(
          "tiles_around_hive",
          [](const Game &game) {
            return encode_tiles(game.board().tiles_around_hive());
          }
      )
      .def(
          "top",
          [](const Game &game, int p, int q) -> nb::object {
            const auto ptr = read_tile(p, q);
            if (game.board().is_empty(ptr)) {
              return nb::none();
            }
            return encode_piece(game.board().get_top(ptr));
          }
      )
      .def(
          "is_empty",
          [](const Game &game, int p, int q) {
            return game.board().is_empty(read_tile(p, q));
          }
      )
      .def(
          "remaining",
          [](const Game &game, int player) {
            const auto remaining = game.board().remaining(read_player(player));

            nb::list result;
            for (std::size_t kind = 0; kind < hive::NUMBER_OF_PIECES; ++kind) {
              const auto it = remaining.find(static_cast<PieceKind>(kind));
              result.append(
                  it == remaining.end() ? 0 : static_cast<int>(it->second)
              );
            }
            return result;
          }
      )
      .def(
          "has_placed",
          [](const Game &game, int player) {
            return game.board().has_placed(read_player(player));
          }
      )
      .def(
          "has_placed_queen",
          [](const Game &game, int player) {
            return game.board().has_placed_queen(read_player(player));
          }
      )
      .def(
          "can_place",
          [](const Game &game, int kind) {
            return game.can_place(read_piece_kind(kind));
          }
      )
      .def(
          "can_place_at",
          [](const Game &game, int p, int q) {
            return game.can_place_at(read_tile(p, q));
          }
      )
      .def(
          "can_move_from",
          [](const Game &game, int p, int q) {
            return game.can_move_from(read_tile(p, q));
          }
      )
      .def(
          "placement_targets",
          [](const Game &game) {
            return encode_tiles(game.placement_targets());
          }
      )
      .def(
          "moves_from",
          [](const Game &game, int p, int q) {
            return encode_moves(game.moves_from(read_tile(p, q)));
          }
      )
      .def(
          "legal_moves",
          [](const Game &game) { return encode_moves(game.legal_moves()); }
      )
      .def("has_legal_move", &Game::has_legal_move)
      .def(
          "is_legal",
          [](const Game &game, const nb::tuple &record) {
            return game.is_legal(read_move(record));
          }
      )
      .def(
          "submit",
          [](Game &game, const nb::tuple &record) {
            return game.submit(read_move(record));
          }
      )
      .def(
          "apply",
          [](Game &game, const nb::tuple &record) {
            game.apply(read_move(record));
          }
      )
      .def("pass_if_stuck", &Game::pass_if_stuck)
      .def("queen_position", [](const Game &game, int player) {
        return encode_optional_tile(game.queen_position(read_player(player)));
      });
}
