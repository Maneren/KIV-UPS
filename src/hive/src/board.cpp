#include <algorithm>
#include <generator>
#include <hive/board.h>
#include <queue>
#include <ranges>
#include <stack>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <utils/match.h>

namespace hive {

namespace {
std::generator<TilePointer> neighboring_cells(TilePointer ptr) {
  for (const auto &[p, q] : DIRECTIONS) {
    co_yield {.p = ptr.p + p, .q = ptr.q + q};
  }
}
} // namespace

std::generator<TilePointer> Board::empty_neighbors(
    TilePointer ptr, std::optional<TilePointer> ignored
) const {
  for (const auto neighbor : neighboring_cells(ptr)) {
    if (is_empty(neighbor, ignored)) {
      co_yield neighbor;
    }
  }
}

std::generator<TilePointer>
Board::neighbors(TilePointer ptr, std::optional<TilePointer> ignored) const {
  for (const auto neighbor : neighboring_cells(ptr)) {
    if (!is_empty(neighbor, ignored)) {
      co_yield neighbor;
    }
  }
}

bool Board::tile_belongs_to_player(
    TilePointer ptr, Player player, std::optional<TilePointer> ignored
) const {
  return get_top(ptr, ignored).owner == player;
}

bool Board::neighbors_only_players(
    TilePointer ptr, Player player, std::optional<TilePointer> ignored
) const {
  return std::ranges::all_of(
      neighbors(ptr, ignored), [player, ignored, this](TilePointer neighbor) {
        return tile_belongs_to_player(neighbor, player, ignored);
      }
  );
}

std::unordered_set<TilePointer>
Board::tiles_around_hive(std::optional<TilePointer> ignored) const {
  std::unordered_set<TilePointer> tiles;

  for (const auto &entry : data) {
    if (is_empty(entry.first, ignored)) {
      continue;
    }

    for (auto neighbor_ptr : empty_neighbors(entry.first, ignored)) {
      tiles.insert(neighbor_ptr);
    }
  }

  return tiles;
}

bool Board::can_player_place_at(Player player, TilePointer ptr) const {
  return is_empty(ptr) &&
         (!has_placed(player) || neighbors_only_players(ptr, player));
}

std::generator<TilePointer> Board::valid_placements(Player player) const {
  for (const auto &ptr : tiles_around_hive()) {
    if (neighbors_only_players(ptr, player)) {
      co_yield ptr;
    }
  }
}

std::generator<Move>
Board::moves_for_piece(TilePointer pos, Piece piece) const {
  constexpr std::array<
      std::generator<Move> (hive::Board::*)(TilePointer) const,
      NUMBER_OF_PIECES>
      PIECE_HANDLERS{
          &Board::queens_moves,
          &Board::spider_moves,
          &Board::beetle_moves,
          &Board::grasshopper_moves,
          &Board::ant_moves
  };

  const auto &handler =
      PIECE_HANDLERS.at(static_cast<std::uint8_t>(piece.kind));

  for (const auto piece_move : (this->*handler)(pos)) {
    co_yield piece_move;
  }
}

std::generator<Move>
Board::moves_for_player(Player player, PlayerPiecesMap pieces) const {
  for (const auto placement_ptr : valid_placements(player)) {
    for (const auto [piece_kind, count] : pieces) {
      if (count == 0) {
        continue;
      }

      co_yield Move{PlacePiece{.pos = placement_ptr, .piece_kind = piece_kind}};
    }
  }

  if (has_placed_queen(player)) {
    for (const auto [pos, piece] : moveable_pieces_for(player)) {
      for (const auto piece_move : moves_for_piece(pos, piece)) {
        co_yield piece_move;
      }
    }
  }
}

std::generator<std::pair<TilePointer, Piece>>
Board::pieces(std::optional<TilePointer> ignored) const {
  for (const auto &entry : data) {
    if (is_empty(entry.first, ignored)) {
      continue;
    }

    co_yield {entry.first, get_top(entry.first, ignored)};
  }
}

std::generator<std::pair<TilePointer, Piece>>
Board::players_tiles(Player player, std::optional<TilePointer> ignored) const {
  for (const auto &entry : data) {
    if (is_empty(entry.first, ignored)) {
      continue;
    }

    const auto top = get_top(entry.first, ignored);
    if (top.owner != player) {
      continue;
    }

    co_yield {entry.first, top};
  }
}

Direction rotate_left(Direction dir) {
  return {dir.first + dir.second, -dir.first};
}

Direction rotate_right(Direction dir) {
  return {-dir.second, dir.first + dir.second};
}

const std::vector<Piece> &Board::get(TilePointer ptr) const {
  return data.at(ptr);
}

Piece Board::get_top(
    TilePointer ptr, std::optional<TilePointer> ignored
) const {
  const auto &stack = get(ptr);
  if (ignored == ptr) {
    // Top piece is treated as absent; reveal the covered one.
    if (stack.size() <= 1) {
      throw std::runtime_error("No pieces at position");
    }
    return stack.at(stack.size() - 2);
  }
  if (stack.empty()) {
    throw std::runtime_error("No pieces at position");
  }
  return stack.back();
}

bool Board::is_empty() const { return data.empty(); }

bool Board::is_empty(
    TilePointer ptr, std::optional<TilePointer> ignored
) const {
  const auto it = data.find(ptr);
  if (it == data.end() || it->second.empty()) {
    return true;
  }
  // An ignored top piece is treated as absent; covered pieces stay.
  if (ignored == ptr) {
    return it->second.size() <= 1;
  }
  return false;
}

bool Board::can_player_move(Player player, TilePointer ptr) const {
  return !is_empty(ptr) && has_placed_queen(player) &&
         get_top(ptr).owner == player && !moving_breaks_hive(ptr);
}

void Board::apply_move(Move move, Player player) {
  match::match(
      move,
      [this](MovePiece movement) {
        const auto piece = remove_piece(movement.from);
        if (piece.kind != movement.piece_kind) {
          throw std::runtime_error("Tried to move piece of different kind");
        }
        add_piece(movement.to, piece);
      },
      [this, player](PlacePiece placement) {
        const auto [pos, kind] = placement;

        auto &available = player_pieces.at(player).at(kind);

        if (available == 0) {
          throw std::runtime_error(
              "Attempted to add piece when no pieces left"
          );
        }

        const auto piece = Piece{.kind = kind, .owner = player};
        add_piece(pos, piece);
        --available;
      }
  );
}

const std::map<Player, PlayerPiecesMap> &Board::get_player_pieces() const {
  return player_pieces;
}

void Board::add_piece(TilePointer ptr, Piece piece) {
  const auto [it, _] = data.emplace(ptr, std::vector<Piece>{});
  it->second.push_back(piece);
}

Piece Board::remove_piece(TilePointer ptr) {
  const auto it = data.find(ptr);
  if (it == data.end() || it->second.empty()) {
    throw std::runtime_error("Tried to remove piece from empty tile");
  }

  auto &[_, pieces] = *it;
  const auto piece = pieces.back();
  pieces.pop_back();

  if (pieces.empty()) {
    data.erase(it);
  }

  return piece;
}

bool Board::moving_breaks_hive(TilePointer ptr) const {
  if (data.size() <= 2) {
    return false;
  }

  const auto &tile = get(ptr);

  if (tile.size() > 1) {
    return false;
  }

  std::unordered_set<TilePointer> visited{};
  std::stack<TilePointer> stack{};

  // start from one of the neighbors (ignoring the lifted piece)
  for (const auto neighbor_ptr : neighbors(ptr, ptr)) {
    stack.push(neighbor_ptr);
    break;
  }

  while (!stack.empty()) {
    const auto current = stack.top();
    stack.pop();

    if (visited.contains(current)) {
      continue;
    }
    visited.insert(current);

    for (const auto neighbor_ptr : neighbors(current, ptr)) {
      stack.push(neighbor_ptr);
    }
  }

  return std::ranges::any_of(
      pieces(ptr) | std::ranges::views::keys,
      [&visited](const auto ptr) { return !visited.contains(ptr); }
  );
}

std::generator<std::pair<TilePointer, Piece>>
Board::moveable_pieces_for(Player player) const {
  // Lift-free: the ignore is an explicit parameter now, so iterating
  // players_tiles() while can_player_move() checks connectivity is safe.
  for (const auto [pos, piece] : players_tiles(player)) {
    if (!can_player_move(player, pos)) {
      continue;
    }

    co_yield {pos, piece};
  }
}

bool Board::can_move_to(
    TilePointer from,
    TilePointer to,
    bool can_leave,
    std::optional<TilePointer> ignored
) const {
  const auto [p, q] = from;
  const auto [np, nq] = to;

  const auto dir = std::make_pair(np - p, nq - q);

  const auto [lp, lq] = rotate_left(dir);
  const auto [rp, rq] = rotate_right(dir);

  const TilePointer left{.p = p + lp, .q = q + lq};
  const TilePointer right{.p = p + rp, .q = q + rq};

  const auto left_empty = is_empty(left, ignored);
  const auto right_empty = is_empty(right, ignored);

  const bool has_gap = left_empty != right_empty;
  const bool both_empty_and_can_leave = left_empty && right_empty && can_leave;
  const bool has_neighbor_in_to_dir =
      has_neighbor_in_direction(to, dir, ignored);

  return has_gap || (both_empty_and_can_leave && has_neighbor_in_to_dir);
}

std::generator<TilePointer> Board::valid_steps(
    TilePointer ptr, std::optional<TilePointer> ignored, bool can_leave
) const {
  for (const auto neighbor : empty_neighbors(ptr, ignored)) {
    if (can_move_to(ptr, neighbor, can_leave, ignored)) {
      co_yield neighbor;
    }
  }
}

bool Board::has_neighbor_in_direction(
    TilePointer cell, Direction direction, std::optional<TilePointer> ignored
) const {
  const auto [p, q] = cell;
  const auto [dp, dq] = direction;

  const auto [lp, lq] = rotate_left(direction);
  const auto [rp, rq] = rotate_right(direction);

  const TilePointer left{.p = p + lp, .q = q + lq};
  const TilePointer right{.p = p + rp, .q = q + rq};
  const TilePointer center{.p = p + dp, .q = q + dq};

  return !is_empty(left, ignored) || !is_empty(right, ignored) ||
         !is_empty(center, ignored);
}

bool Board::has_neighbor(
    TilePointer cell, std::optional<TilePointer> ignored
) const {
  return data.size() == 2 ||
         std::ranges::any_of(
             neighboring_cells(cell), [ignored, this](TilePointer ptr) {
               return !is_empty(ptr, ignored);
             }
         );
}

std::generator<Move> Board::grasshopper_moves(TilePointer grasshopper) const {
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
      if (is_empty(current, grasshopper)) {
        if (skipped) {
          co_yield Move{MovePiece{
              .from = grasshopper,
              .to = current,
              .piece_kind = PieceKind::Grasshopper
          }};
        }
        break;
      }

      skipped = true;
    }
  }
}

std::generator<Move> Board::queens_moves(TilePointer queen) const {
  for (const auto step_ptr : valid_steps(queen, queen, true)) {
    co_yield Move{
        MovePiece{.from = queen, .to = step_ptr, .piece_kind = PieceKind::Queen}
    };
  }
}

std::generator<Move> Board::beetle_moves(TilePointer beetle) const {
  for (const auto ptr : neighboring_cells(beetle)) {
    if (has_neighbor(ptr, beetle)) {
      co_yield Move{
          MovePiece{.from = beetle, .to = ptr, .piece_kind = PieceKind::Beetle}
      };
    }
  }
}

std::generator<Move> Board::spider_moves(TilePointer spider) const {
  std::unordered_set<TilePointer> visited;
  std::vector<TilePointer> stack;
  stack.push_back(spider);

  // do 3 steps
  for (size_t i = 0; i < 3; ++i) {
    std::vector<TilePointer> new_stack;

    for (const auto ptr : stack) {
      visited.insert(ptr);

      for (const auto step_ptr : valid_steps(ptr, spider)) {
        if (!visited.contains(step_ptr)) {
          new_stack.push_back(step_ptr);
        }
      }
    }

    if (new_stack.empty()) {
      break;
    }

    stack = std::move(new_stack);
  }

  for (const auto ptr : stack) {
    co_yield Move{
        MovePiece{.from = spider, .to = ptr, .piece_kind = PieceKind::Spider}
    };
  }
}

std::generator<Move> Board::ant_moves(TilePointer ant) const {
  std::unordered_set<TilePointer> visited{ant};
  std::queue<TilePointer> queue;

  // Add initial neighbors to queue
  for (const auto neighbor : valid_steps(ant, ant)) {
    queue.push(neighbor);
  }

  while (!queue.empty()) {
    const auto current = queue.front();
    queue.pop();

    if (visited.contains(current)) {
      continue;
    }
    visited.insert(current);

    co_yield Move{
        MovePiece{.from = ant, .to = current, .piece_kind = PieceKind::Ant}
    };

    // Add next level neighbors
    for (const auto neighbor : valid_steps(current, ant)) {
      queue.push(neighbor);
    }
  }
}

const PlayerPiecesMap Board::DEFAULT_PLAYER_PIECES = {
    {PieceKind::Queen, 1},
    {PieceKind::Grasshopper, 2},
    {PieceKind::Beetle, 2},
    {PieceKind::Spider, 2},
    {PieceKind::Ant, 2},
};

bool Board::can_player_place(Player player, PieceKind kind) const {
  return player_pieces.at(player).at(kind) > 0;
};

bool Board::has_placed_queen(Player player) const {
  return player_pieces.at(player).at(PieceKind::Queen) == 0;
}

bool Board::has_placed(Player player) const {
  return player_pieces.at(player) != DEFAULT_PLAYER_PIECES;
}

} // namespace hive
