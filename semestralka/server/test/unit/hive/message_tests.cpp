#include "hive/types.h"
#include "utils/serde.h"
#include <gtest/gtest.h>
#include <hive/messages.h>

class MessageTest : public ::testing::Test {
protected:
  void SetUp() override {};

  void TearDown() override {};
};

const hive::MoveMessage TEST_MOVE_MESSAGE{
    hive::make_move(
        hive::TilePointer{.p = 0, .q = 0},
        hive::TilePointer{.p = 1, .q = 1},
        hive::PieceKind::Queen
    ),
    hive::Player::White
};
const std::string_view TEST_MOVE_MESSAGE_STRING = "m;0,0;1,1;Q;W.";
const hive::OkMessage TEST_OK_MESSAGE{};
const std::string_view TEST_OK_MESSAGE_STRING = "k.";

TEST_F(MessageTest, TilePointerSerialize) {
  EXPECT_EQ(serde::serialize(TEST_MOVE_MESSAGE.move().from), "0,0");
  EXPECT_EQ(serde::serialize(TEST_MOVE_MESSAGE.move().to), "1,1");
}

TEST_F(MessageTest, TilePointerDeserialize) {
  serde::DeserializationContext ctx{"0,0"};
  const auto deserialized = serde::deserialize<hive::TilePointer>(ctx);
  EXPECT_TRUE(deserialized.has_value());
  constexpr hive::TilePointer expected{.p = 0, .q = 0};
  EXPECT_EQ(*deserialized, expected);
  EXPECT_TRUE(ctx.empty());
}

TEST_F(MessageTest, MoveMessageSerialize) {
  EXPECT_EQ(serde::serialize(TEST_MOVE_MESSAGE), TEST_MOVE_MESSAGE_STRING);
}

TEST_F(MessageTest, MoveMessageDeserialize) {
  serde::DeserializationContext ctx{TEST_MOVE_MESSAGE_STRING};
  const auto deserialized = serde::deserialize<hive::MoveMessage>(ctx);
  EXPECT_TRUE(deserialized.has_value());
  EXPECT_EQ(*deserialized, TEST_MOVE_MESSAGE);
  EXPECT_TRUE(ctx.empty());
}

TEST_F(MessageTest, OkMessageSerialize) {
  EXPECT_EQ(serde::serialize(TEST_OK_MESSAGE), TEST_OK_MESSAGE_STRING);
}

TEST_F(MessageTest, OkMessageDeserialize) {
  serde::DeserializationContext ctx{TEST_OK_MESSAGE_STRING};
  const auto deserialized = serde::deserialize<hive::OkMessage>(ctx);
  EXPECT_TRUE(deserialized.has_value());
  EXPECT_EQ(*deserialized, TEST_OK_MESSAGE);
  EXPECT_TRUE(ctx.empty());
}
//
// TEST_F(MessageTest, MoveMessageDeserializeInvalid) {
//   serde::Context ctx{"foo"};
//   const auto deserialized = serde::deserialize<hive::MoveMessage>(ctx);
//   EXPECT_FALSE(deserialized.has_value());
// }
