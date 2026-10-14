#include <gtest/gtest.h>
#include <proto/message.h>

namespace {

void expect_round_trip(const proto::Message &msg) {
  const auto frame = proto::encode(msg);
  ASSERT_TRUE(frame.has_value());
  const auto decoded = proto::decode(*frame);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(*decoded, msg);
}

void expect_invalid(std::string_view frame) {
  EXPECT_FALSE(proto::decode(frame).has_value()) << frame;
}

} // namespace

TEST(ProtoTest, LoginWireFormat) {
  const proto::Message msg{proto::Login{.nick = "alice", .lobby = "my-lobby"}};
  EXPECT_EQ(proto::encode(msg), "HIVE LOGN 05alice 08my-lobby\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, PlaceWireFormat) {
  const proto::Message msg{
      proto::Place{.pos = {.p = 0, .q = 0}, .piece = hive::PieceKind::Queen}
  };
  EXPECT_EQ(proto::encode(msg), "HIVE PLAC 0 0 Q\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, MoveWireFormat) {
  const proto::Message msg{proto::Move{
      .from = {.p = 0, .q = 1},
      .to = {.p = 1, .q = 0},
      .piece = hive::PieceKind::Grasshopper
  }};
  EXPECT_EQ(proto::encode(msg), "HIVE MOVE 0 1 1 0 G\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, GameStartWireFormat) {
  const proto::Message msg{
      proto::GameStart{.color = hive::Player::White, .opponent = "bob"}
  };
  EXPECT_EQ(proto::encode(msg), "HIVE STRT W 03bob\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, GameOverWireFormat) {
  const proto::Message msg{proto::GameOver{
      .winner = hive::Player::White, .reason = proto::EndReason::Surround
  }};
  EXPECT_EQ(proto::encode(msg), "HIVE OVER W SURN\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, RejectWireFormat) {
  const proto::Message msg{
      proto::Reject{.code = proto::RejectCode::NotYourTurn}
  };
  EXPECT_EQ(proto::encode(msg), "HIVE REJT NOTN\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, LobbyQueuedWireFormat) {
  const proto::Message msg{
      proto::LobbyStatus{.state = proto::LobbyState::Queued, .position = 1}
  };
  EXPECT_EQ(proto::encode(msg), "HIVE LOBY QUEU 01\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, LobbyListWireFormat) {
  const proto::Message msg{proto::LobbyList{
      .entries = {proto::LobbyEntry{.name = "my-lobby", .occupants = 3}}
  }};
  EXPECT_EQ(proto::encode(msg), "HIVE LIST 01 08my-lobby 03\n");
  expect_round_trip(msg);
}

TEST(ProtoTest, BareCommandsRoundTrip) {
  expect_round_trip(proto::Message{proto::RematchVote{}});
  expect_round_trip(proto::Message{proto::Leave{}});
  expect_round_trip(proto::Message{proto::ListLobbies{}});
  expect_round_trip(proto::Message{proto::Ack{}});
  EXPECT_EQ(proto::encode(proto::Message{proto::Ack{}}), "HIVE ACKN\n");
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
          .state = proto::LobbyState::Waiting, .position = std::nullopt
      }}
  );
  expect_round_trip(
      proto::Message{
          proto::LobbyStatus{.state = proto::LobbyState::Queued, .position = 99}
      }
  );
  expect_round_trip(
      proto::Message{proto::LobbyStatus{
          .state = proto::LobbyState::Promoted, .position = std::nullopt
      }}
  );
  expect_round_trip(
      proto::Message{proto::LobbyStatus{
          .state = proto::LobbyState::RematchWait, .position = std::nullopt
      }}
  );
}

TEST(ProtoTest, NegativeCoordinatesRoundTrip) {
  expect_round_trip(
      proto::Message{proto::Place{
          .pos = {.p = -2147483648, .q = 2147483647},
          .piece = hive::PieceKind::Ant
      }}
  );
}

TEST(ProtoTest, DrawWinnerAccepted) {
  // '-' is reserved in beta but must still parse (forward compatibility).
  const auto decoded = proto::decode("HIVE OVER - SURN\n");
  ASSERT_TRUE(decoded.has_value());
  const proto::Message expected{proto::GameOver{
      .winner = std::nullopt, .reason = proto::EndReason::Surround
  }};
  EXPECT_EQ(*decoded, expected);
}

TEST(ProtoTest, InvalidFramesRejected) {
  expect_invalid("");
  expect_invalid("HIVE");
  expect_invalid("HIVE ACKN");
  expect_invalid("HIVX0004 ACKN\n");
  expect_invalid("hive0004 ACKN\n");
  expect_invalid("HIVE00X4 ACKN\n");
  expect_invalid("HIVE PLAC 0 0 Q");
  expect_invalid("HIVE PLAC 0 0 Q \n");
  expect_invalid("HIVE PLAC  0 0 Q\n");
  expect_invalid("HIVE XXXX\n");
  expect_invalid("HIVE ACKN \n");
  expect_invalid("HIVE REMTX\n");
  expect_invalid("HIVE PLAC 0 0 X\n");
  expect_invalid("HIVE PLAC 0 0 q\n");
  expect_invalid("HIVE MOVE 0 1 1 0 G \n");
  expect_invalid("HIVE STRT X 03bob\n");
  expect_invalid("HIVE OVER W DRAW\n");
  expect_invalid("HIVE OVER W ABRT\n");
  expect_invalid("HIVE LOBY QUEU\n");
  expect_invalid("HIVE LOBY PROM 01\n");
  expect_invalid("HIVE LOBY QUEU 00\n");
  expect_invalid("HIVE REJT XXXX\n");
  expect_invalid("HIVE REJT BADV\n");
  expect_invalid("HIVE LIST 01\n");
  expect_invalid("HIVE LIST 00 X\n");
}

TEST(ProtoTest, InvalidCoordinatesRejected) {
  expect_invalid("HIVE PLAC 9999999999 0 Q\n");
  expect_invalid("HIVE PLAC -9999999999 0 Q\n");
  expect_invalid("HIVE PLAC +1 0 Q\n");
  expect_invalid("HIVE PLAC  0 Q\n");
  expect_invalid("HIVE PLAC 1x 0 Q\n");
  expect_invalid("HIVE PLAC - 0 Q\n");
}

TEST(ProtoTest, InvalidNamesRejected) {
  // '!' is outside [A-Za-z0-9_-]; lengths are consistent on purpose.
  expect_invalid("HIVE LOGN 05al!ce 08my-lobby\n");
  expect_invalid("HIVE LOGN 00 08my-lobby\n");
  expect_invalid("HIVE LOGN 05alice 00\n");
  expect_invalid("HIVE STRT W 04bo b\n");
}

TEST(ProtoTest, EncodeValidatesNames) {
  EXPECT_FALSE(
      proto::encode(
          proto::Message{
              proto::Login{.nick = "this-nick-is-way-too-long", .lobby = "l"}
          }
      )
          .has_value()
  );
  EXPECT_FALSE(
      proto::encode(
          proto::Message{proto::Login{.nick = "bad nick", .lobby = "l"}}
      )
          .has_value()
  );
  EXPECT_FALSE(
      proto::encode(
          proto::Message{
              proto::GameStart{.color = hive::Player::Black, .opponent = ""}
          }
      )
          .has_value()
  );
}

TEST(ProtoTest, EncodeValidatesLobbyStatus) {
  EXPECT_FALSE(
      proto::encode(
          proto::Message{proto::LobbyStatus{
              .state = proto::LobbyState::Queued, .position = std::nullopt
          }}
      ).has_value()
  );
  EXPECT_FALSE(
      proto::encode(
          proto::Message{proto::LobbyStatus{
              .state = proto::LobbyState::Queued, .position = 0
          }}
      )
          .has_value()
  );
  EXPECT_FALSE(
      proto::encode(
          proto::Message{proto::LobbyStatus{
              .state = proto::LobbyState::Waiting, .position = 1
          }}
      )
          .has_value()
  );
}

TEST(ProtoTest, EncodeValidatesLobbyList) {
  proto::LobbyList big;
  for (unsigned i = 0; i < 100; ++i) {
    big.entries.push_back(proto::LobbyEntry{.name = "l", .occupants = 1});
  }
  EXPECT_FALSE(proto::encode(proto::Message{big}).has_value());
  EXPECT_FALSE(
      proto::encode(
          proto::Message{proto::LobbyList{
              .entries = {proto::LobbyEntry{.name = "l", .occupants = 100}}
          }}
      ).has_value()
  );
}

TEST(ProtoTest, TakeFramePartialAndCoalesced) {
  std::string buffer = "HIVE PL";
  auto first = proto::take_frame(buffer);
  EXPECT_EQ(first.status, proto::TakeStatus::Incomplete);
  EXPECT_TRUE(buffer == "HIVE PL");

  buffer += "AC 0 0 Q\nHIVE ACKN\n";
  first = proto::take_frame(buffer);
  ASSERT_EQ(first.status, proto::TakeStatus::Ok);
  EXPECT_EQ(first.frame, "HIVE PLAC 0 0 Q\n");

  auto second = proto::take_frame(buffer);
  ASSERT_EQ(second.status, proto::TakeStatus::Ok);
  EXPECT_EQ(second.frame, "HIVE ACKN\n");
  EXPECT_TRUE(buffer.empty());

  auto third = proto::take_frame(buffer);
  EXPECT_EQ(third.status, proto::TakeStatus::Incomplete);
}

TEST(ProtoTest, TakeFrameDropsBadLines) {
  std::string buffer = "GARBAGE\nHIVE ACKN\n";
  EXPECT_EQ(proto::take_frame(buffer).status, proto::TakeStatus::Error);
  auto next = proto::take_frame(buffer);
  ASSERT_EQ(next.status, proto::TakeStatus::Ok);
  EXPECT_EQ(next.frame, "HIVE ACKN\n");
  EXPECT_TRUE(buffer.empty());

  std::string no_space = "HIVEACKN\nHIVE ACKN\n";
  EXPECT_EQ(proto::take_frame(no_space).status, proto::TakeStatus::Error);
  next = proto::take_frame(no_space);
  ASSERT_EQ(next.status, proto::TakeStatus::Ok);
  EXPECT_EQ(next.frame, "HIVE ACKN\n");

  std::string blank = "\nHIVE ACKN\n";
  EXPECT_EQ(proto::take_frame(blank).status, proto::TakeStatus::Error);
  next = proto::take_frame(blank);
  ASSERT_EQ(next.status, proto::TakeStatus::Ok);
  EXPECT_EQ(next.frame, "HIVE ACKN\n");
}

TEST(ProtoTest, TakeFrameEnforcesLineCap) {
  std::string buffer(16 * 1024 + 1, 'X');
  EXPECT_EQ(proto::take_frame(buffer).status, proto::TakeStatus::Error);
  EXPECT_TRUE(buffer.empty());

  std::string capped = "HIVE " + std::string(16 * 1024, 'X') + "\nHIVE ACKN\n";
  EXPECT_EQ(proto::take_frame(capped).status, proto::TakeStatus::Error);
  auto next = proto::take_frame(capped);
  ASSERT_EQ(next.status, proto::TakeStatus::Ok);
  EXPECT_EQ(next.frame, "HIVE ACKN\n");
}
