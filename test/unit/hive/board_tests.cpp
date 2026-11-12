#include <generator>
#include <gtest/gtest.h>
#include <hive/board.h>
#include <hive/errors.h>
#include <hive/types.h>
#include <set>
#include <utility>
#include <variant>
#include <vector>

namespace {

using namespace hive;

constexpr TilePointer tile(Coordinate p, Coordinate q) {
  return {.p = p, .q = q};
}

Move place(Coordinate p, Coordinate q, PieceKind kind) {
  return make_placement(tile(p, q), kind);
}

Move move(
    Coordinate from_p,
    Coordinate from_q,
    Coordinate to_p,
    Coordinate to_q,
    PieceKind kind
) {
  return make_move(tile(from_p, from_q), tile(to_p, to_q), kind);
}

using Coordinates = std::pair<Coordinate, Coordinate>;

std::set<Coordinates> destinations(std::generator<Move> moves) {
  std::set<Coordinates> result;
  for (const auto &entry : moves) {
    if (const auto *movement = std::get_if<MovePiece>(&entry)) {
      result.emplace(movement->to.p, movement->to.q);
    }
  }
  return result;
}

} // namespace

TEST(BoardTest, PlaceFirstPieceAtOrigin) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Queen), Player::White);

  EXPECT_TRUE(board.has_placed_queen(Player::White));
  EXPECT_EQ(
      board.get_top(tile(0, 0)),
      (Piece{.kind = PieceKind::Queen, .owner = Player::White})
  );
}

TEST(BoardTest, SupplyDecreasesOnPlacement) {
  Board board;
  const auto before = board.remaining(Player::White);

  board.apply_move(place(0, 0, PieceKind::Queen), Player::White);

  const auto after = board.remaining(Player::White);
  EXPECT_EQ(after.at(PieceKind::Queen), before.at(PieceKind::Queen) - 1);
}

TEST(BoardTest, CanPlayerPlaceTracksSupply) {
  Board board;
  EXPECT_TRUE(board.can_player_place(Player::White, PieceKind::Queen));

  board.apply_move(place(0, 0, PieceKind::Queen), Player::White);

  EXPECT_FALSE(board.can_player_place(Player::White, PieceKind::Queen));
  EXPECT_TRUE(board.can_player_place(Player::White, PieceKind::Ant));
}

TEST(BoardTest, EmptyBoardOnlyPlacesViaGame) {
  const Board board;
  EXPECT_TRUE(board.is_empty_all());
  EXPECT_FALSE(board.can_player_place_at(Player::White, tile(0, 0)));
  EXPECT_FALSE(board.can_player_place_at(Player::White, tile(3, 3)));
}

TEST(BoardTest, FirstPlacementAllowsAnyHiveAdjacentTile) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Queen), Player::White);

  EXPECT_TRUE(board.can_player_place_at(Player::Black, tile(1, 0)));
  EXPECT_TRUE(board.can_player_place_at(Player::Black, tile(0, 1)));
  EXPECT_FALSE(board.can_player_place_at(Player::Black, tile(3, 3)));

  std::set<Coordinates> targets;
  for (const auto ptr : board.valid_placements(Player::Black)) {
    targets.emplace(ptr.p, ptr.q);
  }
  EXPECT_EQ(targets.size(), DIRECTIONS.size());
}

TEST(BoardTest, LaterPlacementsRequireOwnNeighbors) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Queen), Player::White);
  board.apply_move(place(1, 0, PieceKind::Queen), Player::Black);

  EXPECT_TRUE(board.can_player_place_at(Player::White, tile(-1, 0)));
  EXPECT_FALSE(board.can_player_place_at(Player::White, tile(0, 1)));
}

TEST(BoardTest, CannotMoveBeforeQueenPlaced) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Ant), Player::White);

  EXPECT_FALSE(board.can_player_move(Player::White, tile(0, 0)));
}

TEST(BoardTest, QueenCanMoveOneStep) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Queen), Player::White);
  board.apply_move(place(1, 0, PieceKind::Ant), Player::Black);

  const auto moves = destinations(
      board.moves_for_piece(tile(0, 0), board.get_top(tile(0, 0)))
  );
  EXPECT_FALSE(moves.empty());
}

TEST(BoardTest, AntCanMoveManySteps) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Ant), Player::White);
  board.apply_move(place(1, 0, PieceKind::Ant), Player::Black);
  board.apply_move(place(2, 0, PieceKind::Ant), Player::Black);

  const auto moves = destinations(
      board.moves_for_piece(tile(0, 0), board.get_top(tile(0, 0)))
  );
  EXPECT_FALSE(moves.empty());
}

TEST(BoardTest, SpiderMovesExactlyThreeSteps) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Spider), Player::White);
  board.apply_move(place(1, 0, PieceKind::Spider), Player::Black);
  board.apply_move(place(2, 0, PieceKind::Spider), Player::Black);

  const auto moves = destinations(
      board.moves_for_piece(tile(0, 0), board.get_top(tile(0, 0)))
  );
  EXPECT_FALSE(moves.empty());
}

TEST(BoardTest, GrasshopperJumpsOverPieces) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Grasshopper), Player::White);
  board.apply_move(place(1, 0, PieceKind::Ant), Player::Black);
  board.apply_move(place(2, 0, PieceKind::Spider), Player::Black);
  board.apply_move(place(3, 0, PieceKind::Beetle), Player::Black);

  const auto moves = destinations(
      board.moves_for_piece(tile(0, 0), board.get_top(tile(0, 0)))
  );
  EXPECT_TRUE(moves.contains(Coordinates{4, 0}));
}

TEST(BoardTest, BeetleMovesOntoNeighbor) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Beetle), Player::White);
  board.apply_move(place(1, 0, PieceKind::Beetle), Player::Black);

  const auto moves = destinations(
      board.moves_for_piece(tile(0, 0), board.get_top(tile(0, 0)))
  );
  EXPECT_TRUE(moves.contains(Coordinates{1, 0}));
}

TEST(BoardTest, MovingBreaksHiveDetected) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Queen), Player::White);
  board.apply_move(place(1, 0, PieceKind::Ant), Player::Black);
  board.apply_move(place(-1, 0, PieceKind::Spider), Player::Black);

  EXPECT_TRUE(board.moving_breaks_hive(tile(0, 0)));
  EXPECT_FALSE(board.can_player_move(Player::White, tile(0, 0)));
}

TEST(BoardTest, StackingRevealsCoveredPiece) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Queen), Player::White);
  board.apply_move(place(0, 0, PieceKind::Beetle), Player::Black);

  ASSERT_EQ(board.get(tile(0, 0)).size(), 2);
  EXPECT_EQ(
      board.get_top(tile(0, 0)),
      (Piece{.kind = PieceKind::Beetle, .owner = Player::Black})
  );
  EXPECT_EQ(
      board.get_top(tile(0, 0), tile(0, 0)),
      (Piece{.kind = PieceKind::Queen, .owner = Player::White})
  );
  EXPECT_FALSE(board.moving_breaks_hive(tile(0, 0)));
}

TEST(BoardTest, StacksAndTilesAreSorted) {
  Board board;
  board.apply_move(place(2, 0, PieceKind::Ant), Player::White);
  board.apply_move(place(0, 0, PieceKind::Queen), Player::Black);
  board.apply_move(place(0, 0, PieceKind::Beetle), Player::White);

  const auto stacks = board.stacks();
  ASSERT_EQ(stacks.size(), 2);
  EXPECT_EQ(stacks.at(0).ptr, tile(0, 0));
  ASSERT_EQ(stacks.at(0).pieces.size(), 2);
  EXPECT_EQ(
      stacks.at(0).pieces.at(0),
      (Piece{.kind = PieceKind::Queen, .owner = Player::Black})
  );
  EXPECT_EQ(
      stacks.at(0).pieces.at(1),
      (Piece{.kind = PieceKind::Beetle, .owner = Player::White})
  );
  EXPECT_EQ(stacks.at(1).ptr, tile(2, 0));

  EXPECT_EQ(board.occupied_tiles(), (std::vector{tile(0, 0), tile(2, 0)}));
}

TEST(BoardTest, SurroundingCountCountsOccupiedNeighbors) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Queen), Player::White);
  board.apply_move(place(0, 1, PieceKind::Ant), Player::Black);
  board.apply_move(place(-1, 1, PieceKind::Spider), Player::Black);
  board.apply_move(place(-1, 0, PieceKind::Beetle), Player::Black);
  board.apply_move(place(0, -1, PieceKind::Grasshopper), Player::Black);
  board.apply_move(place(1, -1, PieceKind::Spider), Player::Black);

  EXPECT_EQ(board.surrounding_count(tile(0, 0)), 5);
}

TEST(BoardTest, MovingWrongKindDoesNotMutateBoard) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Queen), Player::White);

  EXPECT_THROW(
      board.apply_move(move(0, 0, 1, 0, PieceKind::Ant), Player::White),
      IllegalMoveError
  );
  EXPECT_EQ(board.get(tile(0, 0)).size(), 1);
  EXPECT_TRUE(board.is_empty(tile(1, 0)));
}

TEST(BoardTest, MovingFromEmptyTileThrows) {
  Board board;
  EXPECT_THROW(
      board.apply_move(move(5, 5, 1, 0, PieceKind::Queen), Player::White),
      IllegalMoveError
  );
}

TEST(BoardTest, PlacementWithExhaustedSupplyThrows) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Queen), Player::White);

  EXPECT_THROW(
      board.apply_move(place(1, 0, PieceKind::Queen), Player::White),
      IllegalMoveError
  );
}

TEST(BoardTest, GetTopOnEmptyTileThrows) {
  const Board board;
  EXPECT_THROW((void)board.get_top(tile(0, 0)), EmptyTileError);
  EXPECT_THROW((void)board.get(tile(0, 0)), EmptyTileError);
}

TEST(BoardTest, RemovingFromEmptyTileThrows) {
  Board board;
  EXPECT_THROW(board.remove_piece(tile(0, 0)), EmptyTileError);
}
