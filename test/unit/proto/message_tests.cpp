#include <gtest/gtest.h>
#include <proto/message.h>
#include <utils/serde.h>
#include <variant>

namespace {

void expect_round_trip(const proto::Message &msg) {
  const auto frame = serde::serialize(msg);
  const auto decoded = serde::deserialize<proto::Message>(frame);
  ASSERT_TRUE(decoded.has_value()) << frame;
  EXPECT_EQ(*decoded, msg);
}

void expect_invalid(std::string_view frame) {
  EXPECT_FALSE(serde::deserialize<proto::Message>(frame).has_value()) << frame;
}

} // namespace

TEST(ProtoTest, LoginWireFormat) {
  const proto::Message msg{proto::Login{.nick = "alice", .lobby = "my-lobby"}};
  EXPECT_EQ(serde::serialize(msg), "HIVE LOGIN 05alice 08my-lobby\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, PlaceWireFormat) {
  const proto::Message msg{
      proto::Place{.pos = {.p = 0, .q = 0}, .piece = hive::PieceKind::Queen}
  };
  EXPECT_EQ(serde::serialize(msg), "HIVE PLACE 0 0 Q\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, MoveWireFormat) {
  const proto::Message msg{proto::Move{
      .from = {.p = 0, .q = 1},
      .to = {.p = 1, .q = 0},
      .piece = hive::PieceKind::Grasshopper,
  }};
  EXPECT_EQ(serde::serialize(msg), "HIVE MOVE 0 1 1 0 G\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, GameStartWireFormat) {
  const proto::Message msg{
      proto::GameStart{.color = hive::Player::White, .opponent = "bob"}
  };
  EXPECT_EQ(serde::serialize(msg), "HIVE START W 03bob\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, GameOverWireFormat) {
  const proto::Message msg{proto::GameOver{
      .winner = hive::Player::White,
      .reason = proto::EndReason::Surround,
  }};
  EXPECT_EQ(serde::serialize(msg), "HIVE OVER W SURROUND\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, RejectWireFormat) {
  const proto::Message msg{
      proto::Reject{.code = proto::RejectCode::NotYourTurn}
  };
  EXPECT_EQ(serde::serialize(msg), "HIVE REJECT NOT_YOUR_TURN\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, LobbyQueuedWireFormat) {
  const proto::Message msg{
      proto::LobbyStatus{.state = proto::LobbyState::Queued, .position = 1}
  };
  EXPECT_EQ(serde::serialize(msg), "HIVE LOBBY QUEUED 01\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, LobbyListWireFormat) {
  const proto::Message msg{proto::LobbyList{
      .entries = {proto::LobbyEntry{.name = "my-lobby", .occupants = 3}},
  }};
  EXPECT_EQ(serde::serialize(msg), "HIVE LOBBIES 01 08my-lobby 03\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, LobbyListEmptyWireFormat) {
  const proto::Message msg{proto::LobbyList{.entries = {}}};
  EXPECT_EQ(serde::serialize(msg), "HIVE LOBBIES 00\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, BareCommandsRoundTrip) {
  expect_round_trip(proto::Message{proto::RematchVote{}});
  expect_round_trip(proto::Message{proto::Leave{}});
  expect_round_trip(proto::Message{proto::ListLobbies{}});
  expect_round_trip(proto::Message{proto::Ack{}});
  EXPECT_EQ(serde::serialize(proto::Message{proto::Ack{}}), "HIVE ACK\n");
}

TEST(ProtoTest, AllRejectCodesRoundTrip) {
  for (auto code : {
           proto::RejectCode::BadSyntax,
           proto::RejectCode::BadNick,
           proto::RejectCode::NotYourTurn,
           proto::RejectCode::IllegalMove,
           proto::RejectCode::BadState,
       }) {
    expect_round_trip(proto::Message{proto::Reject{.code = code}});
  }
}

TEST(ProtoTest, AllLobbyStatesRoundTrip) {
  expect_round_trip(
      proto::Message{proto::LobbyStatus{
          .state = proto::LobbyState::Waiting,
          .position = std::nullopt,
      }}
  );
  expect_round_trip(
      proto::Message{
          proto::LobbyStatus{.state = proto::LobbyState::Queued, .position = 99}
      }
  );
  expect_round_trip(
      proto::Message{proto::LobbyStatus{
          .state = proto::LobbyState::Promoted,
          .position = std::nullopt,
      }}
  );
  expect_round_trip(
      proto::Message{proto::LobbyStatus{
          .state = proto::LobbyState::RematchWait,
          .position = std::nullopt,
      }}
  );
}

TEST(ProtoTest, NegativeCoordinatesRoundTrip) {
  expect_round_trip(
      proto::Message{proto::Place{
          .pos = {.p = INT32_MIN, .q = INT32_MAX},
          .piece = hive::PieceKind::Ant,
      }}
  );
}

TEST(ProtoTest, InvalidFramesRejected) {
  expect_invalid("");
  expect_invalid("HIVE");
  expect_invalid("HIVE ACK");
  expect_invalid("HIVX0004 ACK\n");
  expect_invalid("hive0004 ACK\n");
  expect_invalid("HIVE00X4 ACK\n");
  expect_invalid("HIVE PLACE 0 0 Q");
  expect_invalid("HIVE PLACE 0 0 Q \n");
  expect_invalid("HIVE PLACE  0 0 Q\n");
  expect_invalid("HIVE XXXX\n");
  expect_invalid("HIVE ACK \n");
  expect_invalid("HIVE REMATCHX\n");
  expect_invalid("HIVE PLACE 0 0 X\n");
  expect_invalid("HIVE PLACE 0 0 q\n");
  expect_invalid("HIVE MOVE 0 1 1 0 G \n");
  expect_invalid("HIVE START X 03bob\n");
  expect_invalid("HIVE OVER W DRAW\n");
  expect_invalid("HIVE OVER W ABRT\n");
  expect_invalid("HIVE LOBBY QUEUED\n");
  expect_invalid("HIVE LOBBY PROMOTED 01\n");
  expect_invalid("HIVE LOBBY QUEUED 00\n");
  expect_invalid("HIVE REJECT XXXX\n");
  expect_invalid("HIVE REJECT BADV\n");
  expect_invalid("HIVE LIST 01\n");
  expect_invalid("HIVE LIST 00 X\n");
  expect_invalid("HIVE LOBBIES\n");
  expect_invalid("HIVE LOBBIES 01\n");
  // Magic is part of deserialization, not framing.
  expect_invalid("GARBAGE\n");
  expect_invalid("HIVEACK\n");
  expect_invalid("\n");
  expect_invalid("HIVE ACK\r\n");
}

TEST(ProtoTest, InvalidCoordinatesRejected) {
  expect_invalid("HIVE PLACE 9999999999 0 Q\n");
  expect_invalid("HIVE PLACE -9999999999 0 Q\n");
  expect_invalid("HIVE PLACE +1 0 Q\n");
  expect_invalid("HIVE PLACE  0 Q\n");
  expect_invalid("HIVE PLACE 1x 0 Q\n");
  expect_invalid("HIVE PLACE - 0 Q\n");
}

TEST(ProtoTest, InvalidNamesRejected) {
  // '!' is outside [A-Za-z0-9_-]; lengths are consistent on purpose.
  expect_invalid("HIVE LOGIN 05al!ce 08my-lobby\n");
  expect_invalid("HIVE LOGIN 00 08my-lobby\n");
  expect_invalid("HIVE LOGIN 05alice 00\n");
  expect_invalid("HIVE START W 04bo b\n");
  // Length prefix must match limits from PROTOCOL.md.
  expect_invalid("HIVE LOGIN 17abcdefghijklmnopq 01l\n");
  expect_invalid("HIVE LOGIN 05alice 33abcdefghijklmnopqrstuvwxyz1234567\n");
}

TEST(ProtoTest, InvalidWireRoundTrip) {
  // Serializing out-of-range values still produces wire the peer rejects.
  const proto::Message bad_nick{
      proto::Login{.nick = "this-nick-is-way-too-long", .lobby = "l"}
  };
  EXPECT_FALSE(
      serde::deserialize<proto::Message>(serde::serialize(bad_nick)).has_value()
  );
}

TEST(ProtoTest, TakeFramePartialAndCoalesced) {
  std::string buffer = "HIVE PL";
  EXPECT_TRUE(
      std::holds_alternative<proto::IncompleteFrame>(proto::take_frame(buffer))
  );
  EXPECT_TRUE(buffer == "HIVE PL");

  buffer += "ACE 0 0 Q\nHIVE ACK\n";
  auto first = proto::take_frame(buffer);
  ASSERT_TRUE(std::holds_alternative<proto::Frame>(first));
  EXPECT_EQ(std::get<proto::Frame>(first).frame, "HIVE PLACE 0 0 Q\n");

  auto second = proto::take_frame(buffer);
  ASSERT_TRUE(std::holds_alternative<proto::Frame>(second));
  EXPECT_EQ(std::get<proto::Frame>(second).frame, "HIVE ACK\n");
  EXPECT_TRUE(buffer.empty());

  EXPECT_TRUE(
      std::holds_alternative<proto::IncompleteFrame>(proto::take_frame(buffer))
  );
}

TEST(ProtoTest, TakeFrameIsPureFraming) {
  // Framing splits on '\n' only; magic validation lives in deserialization.
  std::string buffer = "GARBAGE\nHIVE ACK\n";
  auto first = proto::take_frame(buffer);
  ASSERT_TRUE(std::holds_alternative<proto::Frame>(first));
  EXPECT_EQ(std::get<proto::Frame>(first).frame, "GARBAGE\n");
  EXPECT_FALSE(
      serde::deserialize<proto::Message>(std::get<proto::Frame>(first).frame)
          .has_value()
  );
  auto next = proto::take_frame(buffer);
  ASSERT_TRUE(std::holds_alternative<proto::Frame>(next));
  EXPECT_EQ(std::get<proto::Frame>(next).frame, "HIVE ACK\n");
  EXPECT_TRUE(buffer.empty());

  std::string no_space = "HIVEACK\nHIVE ACK\n";
  first = proto::take_frame(no_space);
  ASSERT_TRUE(std::holds_alternative<proto::Frame>(first));
  EXPECT_FALSE(
      serde::deserialize<proto::Message>(std::get<proto::Frame>(first).frame)
          .has_value()
  );
  next = proto::take_frame(no_space);
  ASSERT_TRUE(std::holds_alternative<proto::Frame>(next));
  EXPECT_EQ(std::get<proto::Frame>(next).frame, "HIVE ACK\n");

  std::string blank = "\nHIVE ACK\n";
  first = proto::take_frame(blank);
  ASSERT_TRUE(std::holds_alternative<proto::Frame>(first));
  EXPECT_EQ(std::get<proto::Frame>(first).frame, "\n");
  EXPECT_FALSE(
      serde::deserialize<proto::Message>(std::get<proto::Frame>(first).frame)
          .has_value()
  );
  next = proto::take_frame(blank);
  ASSERT_TRUE(std::holds_alternative<proto::Frame>(next));
  EXPECT_EQ(std::get<proto::Frame>(next).frame, "HIVE ACK\n");
}

TEST(ProtoTest, TakeFrameEnforcesLineCap) {
  std::string buffer(proto::MAX_LINE + 1, 'X');
  EXPECT_TRUE(
      std::holds_alternative<proto::DroppedFrame>(proto::take_frame(buffer))
  );
  EXPECT_TRUE(buffer.empty());

  auto capped = "HIVE " + std::string(proto::MAX_LINE, 'X') + "\nHIVE ACK\n";
  EXPECT_TRUE(
      std::holds_alternative<proto::DroppedFrame>(proto::take_frame(capped))
  );
  auto next = proto::take_frame(capped);
  ASSERT_TRUE(std::holds_alternative<proto::Frame>(next));
  EXPECT_EQ(std::get<proto::Frame>(next).frame, "HIVE ACK\n");
}
