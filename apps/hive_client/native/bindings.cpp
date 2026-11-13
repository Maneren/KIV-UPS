#include <algorithm>
#include <cstddef>
#include <format>
#include <hive/errors.h>
#include <hive/game.h>
#include <hive/types.h>
#include <limits>
#include <nanobind/nanobind.h>
#include <utility>
#include <variant>
#include <vector>

namespace nb = nanobind;

namespace {

using hive::Coordinate;
using hive::Game;
using hive::GameResult;
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

Coordinate read_coordinate(const nb::object &value, const char *what) {
  long long coordinate = 0;
  try {
    coordinate = nb::cast<long long>(value);
  } catch (const nb::cast_error &) {
    throw nb::type_error(std::format("{} must be an integer", what).c_str());
  }

  if (coordinate < std::numeric_limits<Coordinate>::min() ||
      coordinate > std::numeric_limits<Coordinate>::max()) {
    throw nb::value_error(std::format("{} is out of range", what).c_str());
  }

  return static_cast<Coordinate>(coordinate);
}

int read_enum_value(const nb::object &value, const char *what) {
  try {
    return nb::cast<int>(value);
  } catch (const nb::cast_error &) {
    throw nb::type_error(std::format("{} must be an integer", what).c_str());
  }
}

void check_range(int value, int min, int max, const char *what) {
  if (value < min || value > max) {
    throw nb::value_error(
        std::format("{} must be between {} and {}", what, min, max).c_str()
    );
  }
}

Player read_player(const nb::object &value) {
  const int player = read_enum_value(value, "player");
  check_range(player, 0, 1, "player");
  return static_cast<Player>(player);
}

PieceKind read_piece_kind(const nb::object &value) {
  const int kind = read_enum_value(value, "piece_kind");
  check_range(
      kind, 0, static_cast<int>(hive::NUMBER_OF_PIECES) - 1, "piece_kind"
  );
  return static_cast<PieceKind>(kind);
}

TilePointer read_tile(const nb::object &value) {
  return {
      .p = read_coordinate(value.attr("p"), "p"),
      .q = read_coordinate(value.attr("q"), "q"),
  };
}

Move read_move(const nb::tuple &record) {
  if (nb::len(record) != MOVE_RECORD_SIZE) {
    throw nb::value_error("move record must have 6 elements");
  }

  const int type = read_int(record, 0, "move type");
  const auto from_p = read_coordinate(record[1], "from_p");
  const auto from_q = read_coordinate(record[2], "from_q");
  const auto to_p = read_coordinate(record[3], "to_p");
  const auto to_q = read_coordinate(record[4], "to_q");
  const auto kind = read_piece_kind(record[5]);

  const auto to = TilePointer{.p = to_p, .q = to_q};

  if (type == PLACE) {
    return hive::make_placement(to, kind);
  }
  if (type == MOVE) {
    return hive::make_move(TilePointer{.p = from_p, .q = from_q}, to, kind);
  }

  throw nb::value_error("move type must be 0 (place) or 1 (move)");
}

struct PyTypes {
  PyTypes()
      : tile_pointer(import_attr("hive_client.domain.geometry", "TilePointer")),
        move(import_attr("hive_client.domain.moves", "Move")),
        piece(import_attr("hive_client.domain.pieces", "Piece")),
        piece_kind(import_attr("hive_client.domain.pieces", "PieceKind")),
        player(import_attr("hive_client.domain.pieces", "Player")),
        game_result(import_attr("hive_client.domain.game", "GameResult")) {}

  static nb::object import_attr(const char *module, const char *name) {
    return nb::module_::import_(module).attr(name);
  }

  [[nodiscard]] nb::object make_tile(TilePointer ptr) const {
    return tile_pointer(ptr.p, ptr.q);
  }

  [[nodiscard]] nb::object make_player(Player value) const {
    return player(static_cast<int>(value));
  }

  [[nodiscard]] nb::object make_piece_kind(PieceKind value) const {
    return piece_kind(static_cast<int>(value));
  }

  [[nodiscard]] nb::object make_piece(const Piece &value) const {
    return piece(make_piece_kind(value.kind), make_player(value.owner));
  }

  [[nodiscard]] nb::object make_result(GameResult value) const {
    return game_result(static_cast<int>(value));
  }

  [[nodiscard]] nb::object make_move(const Move &value) const {
    if (const auto *placement = std::get_if<hive::PlacePiece>(&value)) {
      return move(
          PLACE,
          0,
          0,
          placement->pos.p,
          placement->pos.q,
          static_cast<int>(placement->piece_kind)
      );
    }

    const auto &movement = std::get<hive::MovePiece>(value);
    return move(
        MOVE,
        movement.from.p,
        movement.from.q,
        movement.to.p,
        movement.to.q,
        static_cast<int>(movement.piece_kind)
    );
  }

  template <typename Range>
  [[nodiscard]] nb::set make_tile_set(const Range &tiles) const {
    nb::set result;
    for (const auto tile : tiles) {
      result.add(make_tile(tile));
    }
    return result;
  }

  [[nodiscard]] nb::list make_tile_list(std::vector<TilePointer> tiles) const {
    std::ranges::sort(tiles, [](TilePointer lhs, TilePointer rhs) {
      return std::pair{lhs.p, lhs.q} < std::pair{rhs.p, rhs.q};
    });

    nb::list result;
    for (const auto tile : tiles) {
      result.append(make_tile(tile));
    }
    return result;
  }

  [[nodiscard]] nb::list make_moves(const std::vector<Move> &moves) const {
    nb::list result;
    for (const auto &move : moves) {
      result.append(make_move(move));
    }
    return result;
  }

private:
  nb::object tile_pointer;
  nb::object move;
  nb::object piece;
  nb::object piece_kind;
  nb::object player;
  nb::object game_result;
};

} // namespace

// NOLINTNEXTLINE(performance-unnecessary-value-param)
NB_MODULE(hive_native, m) {
  const PyTypes types;

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
          [types](const Game &game) {
            return types.make_player(game.current_player());
          }
      )
      .def_prop_ro(
          "result",
          [types](const Game &game) { return types.make_result(game.result()); }
      )
      .def(
          "is_empty_all",
          [](const Game &game) { return game.board().is_empty_all(); }
      )
      .def(
          "stacks",
          [types](const Game &game) {
            nb::list result;
            for (const auto &stack : game.board().stacks()) {
              nb::list pieces;
              for (const auto &piece : stack.pieces) {
                pieces.append(types.make_piece(piece));
              }
              result.append(nb::make_tuple(types.make_tile(stack.ptr), pieces));
            }
            return result;
          }
      )
      .def(
          "occupied_tiles",
          [types](const Game &game) {
            return types.make_tile_set(game.board().occupied_tiles());
          }
      )
      .def(
          "tiles_around_hive",
          [types](const Game &game) {
            return types.make_tile_set(game.board().tiles_around_hive());
          }
      )
      .def(
          "top",
          [types](const Game &game, const nb::object &tile) {
            return types.make_piece(game.board().get_top(read_tile(tile)));
          }
      )
      .def(
          "is_empty",
          [](const Game &game, const nb::object &tile) {
            return game.board().is_empty(read_tile(tile));
          }
      )
      .def(
          "remaining",
          [types](const Game &game, const nb::object &player) {
            const auto remaining = game.board().remaining(read_player(player));

            nb::dict result;
            for (std::size_t kind = 0; kind < hive::NUMBER_OF_PIECES; ++kind) {
              const auto it = remaining.find(static_cast<PieceKind>(kind));
              result[types.make_piece_kind(static_cast<PieceKind>(kind))] =
                  it == remaining.end() ? 0 : static_cast<int>(it->second);
            }
            return result;
          }
      )
      .def(
          "has_placed_queen",
          [](const Game &game, const nb::object &player) {
            return game.board().has_placed_queen(read_player(player));
          }
      )
      .def(
          "can_place",
          [](const Game &game, const nb::object &kind) {
            return game.can_place(read_piece_kind(kind));
          }
      )
      .def(
          "can_move_from",
          [](const Game &game, const nb::object &tile) {
            return game.can_move_from(read_tile(tile));
          }
      )
      .def(
          "placement_targets",
          [types](const Game &game) {
            return types.make_tile_list(game.placement_targets());
          }
      )
      .def(
          "moves_from",
          [types](const Game &game, const nb::object &tile) {
            return types.make_moves(game.moves_from(read_tile(tile)));
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
      .def("pass_if_stuck", &Game::pass_if_stuck);
}
