#include "hive/board.h"
#include <algorithm>
#include <queue>
#include <ranges>
#include <stack>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace hive {

Board Board::from_fen_string(std::string_view fen) {
  Board board{};

  // auto lines = std::views::split(fen, '/');
  //
  // size_t row_index = 0;
  //
  // for (const auto line : lines) {
  //   if (row_index >= BOARD_SIZE) {
  //     throw std::invalid_argument("Invalid FEN string");
  //   }
  //
  //   std::string_view row_str(line.begin(), line.end());
  //   size_t col_index = 0;
  //
  //   for (const char c : row_str) {
  //     if (col_index >= BOARD_SIZE) {
  //       throw std::invalid_argument("Row too long in FEN string");
  //     }
  //
  //     if (std::isdigit(c) != 0) {
  //       // Skip empty cells (they're already initialized to EMPTY)
  //       col_index += c - '0';
  //     } else if (c == 'X' || c == 'x') {
  //       board.set(col_index, row_index, CellState::X);
  //       col_index++;
  //     } else if (c == 'O' || c == 'o') {
  //       board.set(col_index, row_index, CellState::O);
  //       col_index++;
  //     } else {
  //       throw std::invalid_argument("Invalid character in FEN string");
  //     }
  //   }
  //
  //   row_index++;
  // }

  return board;
}

std::string Board::to_fen_string() const {
  std::string result;

  // for (size_t row = 0; row < BOARD_SIZE; row++) {
  //   size_t empty_cells = 0;
  //
  //   for (size_t col = 0; col < BOARD_SIZE; col++) {
  //     if (get(col, row) == CellState::EMPTY) {
  //       empty_cells++;
  //       continue;
  //     }
  //
  //     if (empty_cells > 0) {
  //       result += std::to_string(empty_cells);
  //       empty_cells = 0;
  //     }
  //
  //     result += get(col, row) == CellState::X ? 'x' : 'o';
  //   }
  //
  //   result += '/';
  // }
  //
  // while (result.back() == '/') {
  //   result.pop_back();
  // }

  return result;
}

std::generator<TilePointer> neighboring_cells(TilePointer ptr) {
  for (const auto &[p, q] : DIRECTIONS) {
    co_yield {.p = ptr.p + p, .q = ptr.q + q};
  }
}

std::generator<TilePointer> Board::empty_neighbors(TilePointer ptr) const {
  for (const auto ptr : neighboring_cells(ptr)) {
    if (is_empty(ptr)) {
      co_yield ptr;
    }
  }
}

std::generator<TilePointer> Board::neighbors(TilePointer ptr) const {
  for (const auto ptr : neighboring_cells(ptr)) {
    if (!is_empty(ptr)) {
      co_yield ptr;
    }
  }
}

bool Board::tile_belongs_to_player(TilePointer ptr, Player player) const {
  return get_top(ptr).owner == player;
}

bool Board::neighbors_only_players(TilePointer ptr, Player player) const {
  return std::ranges::all_of(
      neighbors(ptr), [player, this](TilePointer neighbor) {
        return tile_belongs_to_player(neighbor, player);
      }
  );
}

std::unordered_set<TilePointer, TilePointerHasher>
Board::tiles_around_hive() const {
  std::unordered_set<TilePointer, TilePointerHasher> tiles;

  for (const auto &[pos, tile] : data) {
    if (tile.empty()) {
      continue;
    }

    for (auto neighbor_ptr : neighbors(pos)) {
      tiles.insert(neighbor_ptr);
    }
  }

  return tiles;
}

std::generator<TilePointer> Board::valid_placements(Player player) const {
  for (const auto &ptr : tiles_around_hive()) {
    if (neighbors_only_players(ptr, player)) {
      co_yield ptr;
    }
  }
}

std::generator<Move>
Board::moves_for_player(Player player, PlayerPiecesMap pieces) {
  for (const auto placement_ptr : valid_placements(player)) {
    for (std::size_t i = 0; i < NUMBER_OF_PIECES; ++i) {
      if (pieces.at(i) == 0) {
        continue;
      }

      const auto piece = static_cast<PieceKind>(i);
      co_yield {.from = placement_ptr, .to = placement_ptr, .piece = piece};
    }
  }

  constexpr std::array<
      std::generator<Move> (hive::Board::*)(TilePointer),
      NUMBER_OF_PIECES>
      PIECE_HANDLERS{
          &Board::queens_moves,
          &Board::spider_moves,
          &Board::beetle_moves,
          &Board::grasshopper_moves,
          &Board::ant_moves
      };

  for (const auto [pos, piece] : moveable_pieces_for(player)) {
    auto handler = PIECE_HANDLERS.at(static_cast<std::size_t>(piece.piece));

    for (const auto piece_move : (this->*handler)(pos)) {
      co_yield piece_move;
    }
  }
}

std::generator<std::pair<TilePointer, Piece>>
Board::players_tiles(Player player) const {
  for (const auto &[pos, tile] : data) {
    if (tile.back().owner == player) {
      co_yield {pos, tile.back()};
    }
  }
}

void Board::add_piece(TilePointer ptr, Piece piece) {
  try {
    data.at(ptr).push_back(piece);
  } catch (const std::out_of_range &) {
    data[ptr] = {piece};
  }
}

Piece Board::remove_piece(TilePointer ptr) {
  auto &pieces = data.at(ptr);
  auto piece = pieces.back();
  pieces.pop_back();

  if (pieces.empty()) {
    data.erase(ptr);
  }

  return piece;
}

bool Board::moving_breaks_hive(TilePointer ptr) {
  const auto &pieces = get(ptr);

  if (pieces.size() > 1) {
    return false;
  }

  const LiftPiece _(ptr, this);

  std::unordered_set<TilePointer, TilePointerHasher> visited{};
  std::stack<TilePointer> stack{};
  stack.push(ptr);

  while (!stack.empty()) {
    const auto current = stack.top();
    stack.pop();

    if (visited.contains(current)) {
      continue;
    }
    visited.insert(current);

    for (const auto neighbor_ptr : neighbors(current)) {
      stack.push(neighbor_ptr);
    }
  }

  return std::ranges::all_of(
      data | std::ranges::views::keys,
      [&visited](const auto ptr) { return visited.contains(ptr); }
  );
}

std::generator<std::pair<TilePointer, Piece>>
Board::moveable_pieces_for(Player player) {
  for (const auto [pos, piece] : players_tiles(player)) {
    if (moving_breaks_hive(pos)) {
      continue;
    }

    co_yield {pos, piece};
  }
}

bool Board::can_move_to(
    TilePointer from, TilePointer to, bool can_leave
) const {
  const auto [p, q] = from;
  const auto [np, nq] = to;

  const auto dir = std::make_pair(np - p, nq - q);

  const auto [lp, lq] = rotate_left(dir);
  const auto [rp, rq] = rotate_right(dir);

  const TilePointer left{.p = p + lp, .q = q + lq};
  const TilePointer right{.p = p + rp, .q = q + rq};

  const auto left_empty = is_empty(left);
  const auto right_empty = is_empty(right);

  const bool has_gap = left_empty != right_empty;
  const bool both_empty_and_can_leave = left_empty && right_empty && can_leave;
  const bool has_neighbor_in_to_dir = has_neighbor_in_direction(to, dir);

  return has_gap || (both_empty_and_can_leave && has_neighbor_in_to_dir);
}

std::generator<TilePointer>
Board::valid_steps(TilePointer ptr, bool can_leave) const {
  for (const auto neighbor : empty_neighbors(ptr)) {
    if (can_move_to(ptr, neighbor, can_leave)) {
      co_yield neighbor;
    }
  }
}

bool Board::has_neighbor_in_direction(
    TilePointer cell, Direction direction
) const {
  const auto [p, q] = cell;
  const auto [dp, dq] = direction;

  const auto [lp, lq] = rotate_left(direction);
  const auto [rp, rq] = rotate_right(direction);

  const TilePointer left{.p = p + lp, .q = q + lq};
  const TilePointer right{.p = p + rp, .q = q + rq};
  const TilePointer center{.p = p + dp, .q = q + dq};

  return !is_empty(left) || !is_empty(right) || !is_empty(center);
}

bool Board::has_neighbor(TilePointer cell) const {
  return std::ranges::any_of(neighboring_cells(cell), [this](TilePointer ptr) {
    return !is_empty(ptr);
  });
}

std::generator<Move> Board::grasshopper_moves(TilePointer grasshopper) {
  const LiftPiece _(grasshopper, this);

  // for each direction
  for (const auto [dp, dq] : DIRECTIONS) {
    // start at grasshopper's position
    auto current = grasshopper;
    bool skipped = false;

    // move in that direction until edge of board
    while (true) {
      current = {.p = current.p + dp, .q = current.q + dq};

      // if tile is empty and
      // if something was skipped, yield move
      // else try different direction
      if (is_empty(current)) {
        if (skipped) {
          co_yield Move{
              .from = grasshopper,
              .to = current,
              .piece = PieceKind::Grasshopper
          };
        }
        break;
      }

      skipped = true;
    }
  }
}

std::generator<Move> Board::queens_moves(TilePointer queen) {
  const LiftPiece _(queen, this);

  for (const auto step_ptr : valid_steps(queen, true)) {
    co_yield Move{.from = queen, .to = step_ptr, .piece = PieceKind::Queen};
  }
}

std::generator<Move> Board::beetle_moves(TilePointer beetle) {
  const LiftPiece _(beetle, this);

  for (const auto ptr : neighboring_cells(beetle)) {
    if (has_neighbor(ptr)) {
      co_yield Move{.from = beetle, .to = ptr, .piece = PieceKind::Beetle};
    }
  }
}

std::generator<Move> Board::spider_moves(TilePointer spider) {
  const LiftPiece _(spider, this);

  std::unordered_set<TilePointer> visited;
  std::vector<TilePointer> stack;
  stack.push_back(spider);

  // do 3 steps
  for (size_t i = 0; i < 3; ++i) {
    std::vector<TilePointer> new_stack;

    for (const auto ptr : stack) {
      visited.insert(ptr);

      for (const auto step_ptr : valid_steps(ptr)) {
        new_stack.push_back(step_ptr);
      }
    }

    if (new_stack.empty()) {
      break;
    }

    stack = std::move(new_stack);
  }

  for (const auto ptr : stack) {
    co_yield Move{.from = spider, .to = ptr, .piece = PieceKind::Spider};
  }
}

std::generator<Move> Board::ant_moves(TilePointer ant) {
  const LiftPiece _(ant, this);

  std::unordered_set<TilePointer> visited{ant};
  std::queue<TilePointer> queue;

  // Add initial neighbors to queue
  for (const auto neighbor : valid_steps(ant)) {
    queue.push(neighbor);
  }

  while (!queue.empty()) {
    const auto current = queue.front();
    queue.pop();

    if (visited.contains(current)) {
      continue;
    }
    visited.insert(current);

    co_yield Move{.from = ant, .to = current, .piece = PieceKind::Ant};

    // Add next level neighbors
    for (const auto neighbor : valid_steps(current)) {
      queue.push(neighbor);
    }
  }
}

} // namespace hive
