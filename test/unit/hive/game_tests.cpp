#include <algorithm>
#include <array>
#include <cstddef>
#include <gtest/gtest.h>
#include <hive/board.h>
#include <hive/game.h>
#include <hive/types.h>
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

constexpr std::array<TilePointer, 6> NEIGHBORS{
    tile(1, 0),
    tile(0, 1),
    tile(-1, 1),
    tile(-1, 0),
    tile(0, -1),
    tile(1, -1),
};

Board surrounded_queen_board(Player queen_owner) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Queen), queen_owner);

  const std::array<PieceKind, 6> kinds{
      PieceKind::Ant,
      PieceKind::Ant,
      PieceKind::Spider,
      PieceKind::Spider,
      PieceKind::Beetle,
      PieceKind::Beetle,
  };
  const auto other =
      queen_owner == Player::White ? Player::Black : Player::White;

  for (std::size_t i = 0; i < NEIGHBORS.size(); ++i) {
    board.apply_move(make_placement(NEIGHBORS.at(i), kinds.at(i)), other);
  }

  return board;
}

} // namespace

TEST(GameTest, InitialState) {
  const Game game;

  EXPECT_EQ(game.result(), GameResult::Ongoing);
  EXPECT_EQ(game.current_player(), Player::White);
  EXPECT_TRUE(game.board().is_empty_all());
  EXPECT_FALSE(game.is_over());
  EXPECT_FALSE(game.winner().has_value());
}

TEST(GameTest, EmptyBoardOpeningPlacement) {
  const Game game;

  EXPECT_TRUE(game.can_place(PieceKind::Queen));
  EXPECT_TRUE(game.can_place_at(tile(0, 0)));
  EXPECT_FALSE(game.can_place_at(tile(1, 0)));
  EXPECT_EQ(game.placement_targets(), (std::vector{tile(0, 0)}));
  EXPECT_TRUE(game.legal_moves().empty());
  EXPECT_FALSE(game.has_legal_move());
}

TEST(GameTest, SubmitRejectsPlacementAwayFromOrigin) {
  Game game;

  EXPECT_FALSE(game.submit(place(1, 0, PieceKind::Queen)));
  EXPECT_EQ(game.current_player(), Player::White);
  EXPECT_TRUE(game.board().is_empty_all());

  EXPECT_TRUE(game.submit(place(0, 0, PieceKind::Queen)));
  EXPECT_EQ(game.current_player(), Player::Black);
}

TEST(GameTest, ApplySwitchesPlayer) {
  Game game;
  game.apply(place(0, 0, PieceKind::Queen));

  EXPECT_EQ(game.current_player(), Player::Black);
  EXPECT_EQ(game.result(), GameResult::Ongoing);
}

TEST(GameTest, FirstPlacementAfterOpeningIsAvailable) {
  Game game;
  game.apply(place(0, 0, PieceKind::Queen));

  EXPECT_TRUE(game.can_place_at(tile(1, 0)));
  EXPECT_TRUE(game.can_place_at(tile(0, 1)));
  EXPECT_FALSE(game.can_place_at(tile(3, 3)));
  EXPECT_FALSE(game.placement_targets().empty());
  EXPECT_TRUE(game.has_legal_move());
  EXPECT_FALSE(game.legal_moves().empty());
}

TEST(GameTest, CannotMoveBeforeQueenPlaced) {
  Game game;
  game.apply(place(0, 0, PieceKind::Ant));
  game.apply(place(1, 0, PieceKind::Ant));

  EXPECT_EQ(game.current_player(), Player::White);
  EXPECT_FALSE(game.can_move_from(tile(0, 0)));
  EXPECT_TRUE(game.moves_from(tile(0, 0)).empty());
}

TEST(GameTest, OpeningRestrictionIsNotEnforcedByApply) {
  Game game;
  game.apply(place(1, 0, PieceKind::Queen));

  EXPECT_EQ(game.current_player(), Player::Black);
  EXPECT_EQ(game.board().get_top(tile(1, 0)).owner, Player::White);
}

TEST(GameTest, LegalMovesAreSortedAndStable) {
  Game game;
  game.apply(place(0, 0, PieceKind::Queen));

  const auto first = game.legal_moves();
  const auto second = game.legal_moves();

  EXPECT_EQ(first, second);
  EXPECT_EQ(first.size(), NEIGHBORS.size() * 5);
  EXPECT_TRUE(std::ranges::all_of(first, [](const Move &move) {
    return std::holds_alternative<PlacePiece>(move);
  }));
}

TEST(GameTest, SubmitRejectsIllegalMove) {
  Game game;
  game.apply(place(0, 0, PieceKind::Queen));
  game.apply(place(1, 0, PieceKind::Queen));

  EXPECT_FALSE(
      game.submit(make_move(tile(0, 0), tile(2, 0), PieceKind::Queen))
  );
  EXPECT_EQ(game.current_player(), Player::White);
}

TEST(GameTest, QueenSurroundedMeansOpponentWins) {
  const auto board = surrounded_queen_board(Player::White);
  const Game game(board);

  EXPECT_TRUE(game.is_over());
  EXPECT_EQ(game.result(), GameResult::BlackWins);
  EXPECT_EQ(game.winner(), Player::Black);
}

TEST(GameTest, BlackQueenSurroundedMeansWhiteWins) {
  const auto board = surrounded_queen_board(Player::Black);
  const Game game(board);

  EXPECT_TRUE(game.is_over());
  EXPECT_EQ(game.result(), GameResult::WhiteWins);
  EXPECT_EQ(game.winner(), Player::White);
}

TEST(GameTest, QueenPosition) {
  const auto board = surrounded_queen_board(Player::White);
  const Game game(board);

  EXPECT_EQ(game.queen_position(Player::White), tile(0, 0));
  EXPECT_FALSE(game.queen_position(Player::Black).has_value());
}

TEST(GameTest, PassIfStuckOnEmptyBoard) {
  Game game;
  EXPECT_FALSE(game.pass_if_stuck());
  EXPECT_EQ(game.current_player(), Player::White);
}

TEST(GameTest, PassIfStuckWhenLegalMovesExist) {
  Game game;
  game.apply(place(0, 0, PieceKind::Queen));

  EXPECT_FALSE(game.pass_if_stuck());
  EXPECT_EQ(game.current_player(), Player::Black);
}

TEST(GameTest, PassIfStuckSwitchesPlayer) {
  Board board;
  board.apply_move(place(0, 0, PieceKind::Queen), Player::White);
  board.apply_move(place(0, 1, PieceKind::Ant), Player::Black);
  board.apply_move(place(-1, 1, PieceKind::Spider), Player::Black);
  board.apply_move(place(-1, 0, PieceKind::Beetle), Player::Black);
  board.apply_move(place(0, -1, PieceKind::Grasshopper), Player::Black);
  board.apply_move(place(1, -1, PieceKind::Spider), Player::Black);

  Game game(board, Player::White);
  ASSERT_FALSE(game.is_over());
  ASSERT_FALSE(game.has_legal_move());

  EXPECT_TRUE(game.pass_if_stuck());
  EXPECT_EQ(game.current_player(), Player::Black);
}
