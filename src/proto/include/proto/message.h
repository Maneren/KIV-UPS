#pragma once

#include <algorithm>
#include <cstdint>
#include <format>
#include <hive/types.h>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <utils/match.h>
#include <utils/parse.h>
#include <utils/serde.h>
#include <variant>
#include <vector>

namespace proto {

constexpr std::string_view MAGIC = "HIVE";
constexpr std::size_t MAX_NICK_LEN = 16;
constexpr std::size_t MAX_LOBBY_LEN = 32;
constexpr std::size_t MAX_BODY_LEN = 9999;
constexpr std::size_t MAX_LINE = 16UZ * 1024UZ;
constexpr unsigned MAX_COUNT = 99;

enum class RejectCode : std::uint8_t {
  BadSyntax,
  BadNick,
  NotYourTurn,
  IllegalMove,
  BadState,
};

enum class EndReason : std::uint8_t {
  Surround,
  Forfeit,
};

enum class LobbyState : std::uint8_t {
  Waiting,
  Queued,
  Promoted,
  RematchWait,
};

struct Login {
  std::string nick;
  std::string lobby;

  bool operator==(const Login &) const = default;
};

struct Place {
  hive::TilePointer pos;
  hive::PieceKind piece;

  bool operator==(const Place &) const = default;
};

struct Move {
  hive::TilePointer from;
  hive::TilePointer to;
  hive::PieceKind piece;

  bool operator==(const Move &) const = default;
};

struct RematchVote {
  bool operator==(const RematchVote &) const = default;
};

struct Leave {
  bool operator==(const Leave &) const = default;
};

struct ListLobbies {
  bool operator==(const ListLobbies &) const = default;
};

struct Ack {
  bool operator==(const Ack &) const = default;
};

struct Reject {
  RejectCode code;

  bool operator==(const Reject &) const = default;
};

struct GameStart {
  hive::Player color;
  std::string opponent;

  bool operator==(const GameStart &) const = default;
};

struct GameOver {
  hive::Player winner;
  EndReason reason;

  bool operator==(const GameOver &) const = default;
};

struct LobbyStatus {
  LobbyState state;
  // Queue position, 1-based. Set iff state == Queued.
  std::optional<unsigned> position;

  bool operator==(const LobbyStatus &) const = default;
};

struct LobbyEntry {
  std::string name;
  unsigned occupants;

  bool operator==(const LobbyEntry &) const = default;
};

struct LobbyList {
  std::vector<LobbyEntry> entries;

  bool operator==(const LobbyList &) const = default;
};

using Message = std::variant<
    Login,
    Place,
    Move,
    RematchVote,
    Leave,
    ListLobbies,
    Ack,
    Reject,
    GameStart,
    GameOver,
    LobbyStatus,
    LobbyList>;

struct Frame {
  std::string frame;

  bool operator==(const Frame &) const = default;
};

struct IncompleteFrame {
  bool operator==(const IncompleteFrame &) const = default;
};

struct DroppedFrame {
  bool operator==(const DroppedFrame &) const = default;
};

using TakeResult = std::variant<Frame, IncompleteFrame, DroppedFrame>;

// Read up to MAX_LINE bytes from the buffer until a newline is found.
//
// Removes the read bytes from the buffer.
TakeResult take_frame(std::string &buffer);

namespace wire {

constexpr bool is_name_char(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') || c == '_' || c == '-';
}

inline auto write_sep(auto it) { return std::format_to(it, " "); }

inline bool read_sep(auto &ctx) { return ctx.assert_prefix(' '); }

// Fixed two-digit count, 00-99.
inline auto write_fixed2(unsigned value, auto it) {
  return std::format_to(it, "{:02}", value);
}

inline std::optional<unsigned> read_fixed2(auto &ctx) {
  if (ctx.size() < 2) {
    return std::nullopt;
  }
  const auto value =
      utils::parse_number<unsigned>(std::string_view(ctx.data(), 2));
  if (!value.has_value()) {
    return std::nullopt;
  }
  ctx.remove_prefix(2);
  return value;
}

// Length-prefixed name: 2-digit len + raw chars.
inline auto write_name(std::string_view name, auto it) {
  return std::format_to(it, "{:02}{}", name.size(), name);
}

inline std::optional<std::string> read_name(auto &ctx, std::size_t max_len) {
  serde::DeserializationTransaction tx(ctx);
  const auto len = read_fixed2(ctx);
  if (!len.has_value() || *len == 0 || *len > max_len) {
    return std::nullopt;
  }
  if (ctx.size() < *len) {
    return std::nullopt;
  }
  const auto view = std::string_view(ctx.data(), *len);
  if (!std::ranges::all_of(view, is_name_char)) {
    return std::nullopt;
  }
  std::string out(view);
  ctx.remove_prefix(*len);
  tx.commit();
  return out;
}

inline auto write_coord(hive::Coordinate value, auto it) {
  return std::format_to(it, "{}", value);
}

inline auto write_pos(hive::TilePointer pos, auto it) {
  it = write_coord(pos.p, it);
  it = write_sep(it);
  return write_coord(pos.q, it);
}

// Variable-width int32 coordinate
inline std::optional<hive::Coordinate> read_coord(auto &ctx) {
  const auto token = ctx.substr(0, ctx.find(' '));
  const auto value = utils::parse_number<hive::Coordinate>(token);
  if (!value.has_value()) {
    return std::nullopt;
  }
  ctx.remove_prefix(token.size());
  return value;
}

inline std::optional<hive::TilePointer> read_pos(auto &ctx) {
  serde::DeserializationTransaction tx(ctx);
  auto p = read_coord(ctx);
  if (!p.has_value() || !read_sep(ctx)) {
    return std::nullopt;
  }
  auto q = read_coord(ctx);
  if (!q.has_value()) {
    return std::nullopt;
  }
  tx.commit();
  return hive::TilePointer{.p = *p, .q = *q};
}

} // namespace wire

// Keyword spelling tables shared by the enum serializers/deserializers.
namespace keywords {

inline constexpr std::pair<std::string_view, hive::Player> PLAYER[] = {
    {"W", hive::Player::White},
    {"B", hive::Player::Black},
};

inline constexpr std::pair<std::string_view, hive::PieceKind> PIECE[] = {
    {"Q", hive::PieceKind::Queen},
    {"S", hive::PieceKind::Spider},
    {"B", hive::PieceKind::Beetle},
    {"G", hive::PieceKind::Grasshopper},
    {"A", hive::PieceKind::Ant},
};

inline constexpr std::pair<std::string_view, RejectCode> REJECT_CODE[] = {
    {"BAD_SYNTAX", RejectCode::BadSyntax},
    {"BAD_NAME", RejectCode::BadNick},
    {"NOT_YOUR_TURN", RejectCode::NotYourTurn},
    {"ILLEGAL_MOVE", RejectCode::IllegalMove},
    {"BAD_STATE", RejectCode::BadState},
};

inline constexpr std::pair<std::string_view, EndReason> END_REASON[] = {
    {"SURROUND", EndReason::Surround},
    {"FORFEIT", EndReason::Forfeit},
};

inline constexpr std::pair<std::string_view, LobbyState> LOBBY_STATE[] = {
    {"WAITING", LobbyState::Waiting},
    {"QUEUED", LobbyState::Queued},
    {"PROMOTED", LobbyState::Promoted},
    {"REMATCH_WAIT", LobbyState::RematchWait},
};

} // namespace keywords

} // namespace proto

template <typename O> struct serde::serializer<hive::Player, O> {
  static auto serialize_to(hive::Player msg, auto &ctx) {
    return serde::write_keyword(msg, ctx.out(), proto::keywords::PLAYER);
  }
};

template <> struct serde::deserializer<hive::Player> {
  static std::optional<hive::Player> deserialize(auto &ctx) {
    return serde::read_keyword<hive::Player>(ctx, proto::keywords::PLAYER);
  }
};

template <typename O> struct serde::serializer<hive::PieceKind, O> {
  static auto serialize_to(hive::PieceKind msg, auto &ctx) {
    return serde::write_keyword(msg, ctx.out(), proto::keywords::PIECE);
  }
};

template <> struct serde::deserializer<hive::PieceKind> {
  static std::optional<hive::PieceKind> deserialize(auto &ctx) {
    return serde::read_keyword<hive::PieceKind>(ctx, proto::keywords::PIECE);
  }
};

template <typename O> struct serde::serializer<hive::TilePointer, O> {
  static auto serialize_to(hive::TilePointer msg, auto &ctx) {
    return proto::wire::write_pos(msg, ctx.out());
  }
};

template <> struct serde::deserializer<hive::TilePointer> {
  static std::optional<hive::TilePointer> deserialize(auto &ctx) {
    return proto::wire::read_pos(ctx);
  }
};

template <typename O> struct serde::serializer<proto::RejectCode, O> {
  static auto serialize_to(proto::RejectCode msg, auto &ctx) {
    return serde::write_keyword(msg, ctx.out(), proto::keywords::REJECT_CODE);
  }
};

template <> struct serde::deserializer<proto::RejectCode> {
  static std::optional<proto::RejectCode> deserialize(auto &ctx) {
    return serde::read_keyword<proto::RejectCode>(
        ctx, proto::keywords::REJECT_CODE
    );
  }
};

template <typename O> struct serde::serializer<proto::EndReason, O> {
  static auto serialize_to(proto::EndReason msg, auto &ctx) {
    return serde::write_keyword(msg, ctx.out(), proto::keywords::END_REASON);
  }
};

template <> struct serde::deserializer<proto::EndReason> {
  static std::optional<proto::EndReason> deserialize(auto &ctx) {
    return serde::read_keyword<proto::EndReason>(
        ctx, proto::keywords::END_REASON
    );
  }
};

template <typename O> struct serde::serializer<proto::LobbyState, O> {
  static auto serialize_to(proto::LobbyState msg, auto &ctx) {
    return serde::write_keyword(msg, ctx.out(), proto::keywords::LOBBY_STATE);
  }
};

template <> struct serde::deserializer<proto::LobbyState> {
  static std::optional<proto::LobbyState> deserialize(auto &ctx) {
    return serde::read_keyword<proto::LobbyState>(
        ctx, proto::keywords::LOBBY_STATE
    );
  }
};

template <typename O> struct serde::serializer<proto::Login, O> {
  static auto serialize_to(const proto::Login &msg, auto &ctx) {
    auto it = ctx.out();
    it = std::format_to(it, "LOGIN ");
    it = proto::wire::write_name(msg.nick, it);
    it = proto::wire::write_sep(it);
    return proto::wire::write_name(msg.lobby, it);
  }
};

template <> struct serde::deserializer<proto::Login> {
  static std::optional<proto::Login> deserialize(auto &ctx) {
    serde::DeserializationTransaction tx(ctx);
    if (!ctx.assert_prefix("LOGIN")) {
      return std::nullopt;
    }
    if (!proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto nick = proto::wire::read_name(ctx, proto::MAX_NICK_LEN);
    if (!nick.has_value() || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto lobby = proto::wire::read_name(ctx, proto::MAX_LOBBY_LEN);
    if (!lobby.has_value()) {
      return std::nullopt;
    }
    tx.commit();
    return proto::Login{.nick = *nick, .lobby = *lobby};
  }
};

template <typename O> struct serde::serializer<proto::Place, O> {
  static auto serialize_to(const proto::Place &msg, auto &ctx) {
    auto it = ctx.out();
    it = std::format_to(it, "PLACE ");
    it = proto::wire::write_pos(msg.pos, it);
    it = proto::wire::write_sep(it);
    return serde::write_keyword(msg.piece, it, proto::keywords::PIECE);
  }
};

template <> struct serde::deserializer<proto::Place> {
  static std::optional<proto::Place> deserialize(auto &ctx) {
    serde::DeserializationTransaction tx(ctx);
    if (!ctx.assert_prefix("PLACE") || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto pos = serde::deserialize<hive::TilePointer>(ctx);
    if (!pos.has_value() || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto piece = serde::deserialize<hive::PieceKind>(ctx);
    if (!piece.has_value()) {
      return std::nullopt;
    }
    tx.commit();
    return proto::Place{.pos = *pos, .piece = *piece};
  }
};

template <typename O> struct serde::serializer<proto::Move, O> {
  static auto serialize_to(const proto::Move &msg, auto &ctx) {
    auto it = ctx.out();
    it = std::format_to(it, "MOVE ");
    it = proto::wire::write_pos(msg.from, it);
    it = proto::wire::write_sep(it);
    it = proto::wire::write_pos(msg.to, it);
    it = proto::wire::write_sep(it);
    return serde::write_keyword(msg.piece, it, proto::keywords::PIECE);
  }
};

template <> struct serde::deserializer<proto::Move> {
  static std::optional<proto::Move> deserialize(auto &ctx) {
    serde::DeserializationTransaction tx(ctx);
    if (!ctx.assert_prefix("MOVE") || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto from = serde::deserialize<hive::TilePointer>(ctx);
    if (!from.has_value() || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto to = serde::deserialize<hive::TilePointer>(ctx);
    if (!to.has_value() || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto piece = serde::deserialize<hive::PieceKind>(ctx);
    if (!piece.has_value()) {
      return std::nullopt;
    }
    tx.commit();
    return proto::Move{.from = *from, .to = *to, .piece = *piece};
  }
};

template <typename O> struct serde::serializer<proto::RematchVote, O> {
  static auto serialize_to(auto /*unused*/, auto &ctx) {
    return std::format_to(ctx.out(), "REMATCH");
  }
};

template <> struct serde::deserializer<proto::RematchVote> {
  static std::optional<proto::RematchVote> deserialize(auto &ctx) {
    if (!ctx.assert_prefix("REMATCH")) {
      return std::nullopt;
    }
    return proto::RematchVote{};
  }
};

template <typename O> struct serde::serializer<proto::Leave, O> {
  static auto serialize_to(auto /*unused*/, auto &ctx) {
    return std::format_to(ctx.out(), "LEAVE");
  }
};

template <> struct serde::deserializer<proto::Leave> {
  static std::optional<proto::Leave> deserialize(auto &ctx) {
    if (!ctx.assert_prefix("LEAVE")) {
      return std::nullopt;
    }
    return proto::Leave{};
  }
};

template <typename O> struct serde::serializer<proto::ListLobbies, O> {
  static auto serialize_to(auto /*unused*/, auto &ctx) {
    return std::format_to(ctx.out(), "LIST");
  }
};

template <> struct serde::deserializer<proto::ListLobbies> {
  static std::optional<proto::ListLobbies> deserialize(auto &ctx) {
    if (!ctx.assert_prefix("LIST")) {
      return std::nullopt;
    }
    return proto::ListLobbies{};
  }
};

template <typename O> struct serde::serializer<proto::Ack, O> {
  static auto serialize_to(auto /*unused*/, auto &ctx) {
    return std::format_to(ctx.out(), "ACK");
  }
};

template <> struct serde::deserializer<proto::Ack> {
  static std::optional<proto::Ack> deserialize(auto &ctx) {
    if (!ctx.assert_prefix("ACK")) {
      return std::nullopt;
    }
    return proto::Ack{};
  }
};

template <typename O> struct serde::serializer<proto::Reject, O> {
  static auto serialize_to(const proto::Reject &msg, auto &ctx) {
    auto it = ctx.out();
    it = std::format_to(it, "REJECT ");
    return serde::write_keyword(msg.code, it, proto::keywords::REJECT_CODE);
  }
};

template <> struct serde::deserializer<proto::Reject> {
  static std::optional<proto::Reject> deserialize(auto &ctx) {
    serde::DeserializationTransaction tx(ctx);
    if (!ctx.assert_prefix("REJECT") || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto code = serde::deserialize<proto::RejectCode>(ctx);
    if (!code.has_value()) {
      return std::nullopt;
    }
    tx.commit();
    return proto::Reject{.code = *code};
  }
};

template <typename O> struct serde::serializer<proto::GameStart, O> {
  static auto serialize_to(const proto::GameStart &msg, auto &ctx) {
    auto it = ctx.out();
    it = std::format_to(it, "START ");
    it = serde::write_keyword(msg.color, it, proto::keywords::PLAYER);
    it = proto::wire::write_sep(it);
    return proto::wire::write_name(msg.opponent, it);
  }
};

template <> struct serde::deserializer<proto::GameStart> {
  static std::optional<proto::GameStart> deserialize(auto &ctx) {
    serde::DeserializationTransaction tx(ctx);
    if (!ctx.assert_prefix("START") || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto color = serde::deserialize<hive::Player>(ctx);
    if (!color.has_value() || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto opponent = proto::wire::read_name(ctx, proto::MAX_NICK_LEN);
    if (!opponent.has_value()) {
      return std::nullopt;
    }
    tx.commit();
    return proto::GameStart{.color = *color, .opponent = *opponent};
  }
};

template <typename O> struct serde::serializer<proto::GameOver, O> {
  static auto serialize_to(const proto::GameOver &msg, auto &ctx) {
    auto it = ctx.out();
    it = std::format_to(it, "OVER ");
    it = serde::write_keyword(msg.winner, it, proto::keywords::PLAYER);
    it = proto::wire::write_sep(it);
    return serde::write_keyword(msg.reason, it, proto::keywords::END_REASON);
  }
};

template <> struct serde::deserializer<proto::GameOver> {
  static std::optional<proto::GameOver> deserialize(auto &ctx) {
    serde::DeserializationTransaction tx(ctx);
    if (!ctx.assert_prefix("OVER") || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto winner = serde::deserialize<hive::Player>(ctx);
    if (!winner.has_value() || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto reason = serde::deserialize<proto::EndReason>(ctx);
    if (!reason.has_value()) {
      return std::nullopt;
    }
    tx.commit();
    return proto::GameOver{.winner = *winner, .reason = *reason};
  }
};

template <typename O> struct serde::serializer<proto::LobbyStatus, O> {
  static auto serialize_to(const proto::LobbyStatus &msg, auto &ctx) {
    auto it = ctx.out();
    it = std::format_to(it, "LOBBY ");
    it = serde::write_keyword(msg.state, it, proto::keywords::LOBBY_STATE);
    if (msg.state == proto::LobbyState::Queued) {
      it = proto::wire::write_sep(it);
      it = proto::wire::write_fixed2(msg.position.value_or(0), it);
    }
    return it;
  }
};

template <> struct serde::deserializer<proto::LobbyStatus> {
  static std::optional<proto::LobbyStatus> deserialize(auto &ctx) {
    serde::DeserializationTransaction tx(ctx);
    if (!ctx.assert_prefix("LOBBY") || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto state = serde::deserialize<proto::LobbyState>(ctx);
    if (!state.has_value()) {
      return std::nullopt;
    }
    if (*state == proto::LobbyState::Queued) {
      if (!proto::wire::read_sep(ctx)) {
        return std::nullopt;
      }
      auto pos = proto::wire::read_fixed2(ctx);
      if (!pos.has_value() || *pos < 1 || *pos > proto::MAX_COUNT) {
        return std::nullopt;
      }
      tx.commit();
      return proto::LobbyStatus{.state = *state, .position = *pos};
    }
    tx.commit();
    return proto::LobbyStatus{.state = *state, .position = std::nullopt};
  }
};

template <typename O> struct serde::serializer<proto::LobbyEntry, O> {
  static auto serialize_to(const proto::LobbyEntry &msg, auto &ctx) {
    auto it = ctx.out();
    it = proto::wire::write_name(msg.name, it);
    it = proto::wire::write_sep(it);
    return proto::wire::write_fixed2(msg.occupants, it);
  }
};

template <> struct serde::deserializer<proto::LobbyEntry> {
  static std::optional<proto::LobbyEntry> deserialize(auto &ctx) {
    serde::DeserializationTransaction tx(ctx);
    auto name = proto::wire::read_name(ctx, proto::MAX_LOBBY_LEN);
    if (!name.has_value() || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto occupants = proto::wire::read_fixed2(ctx);
    if (!occupants.has_value() || *occupants > proto::MAX_COUNT) {
      return std::nullopt;
    }
    tx.commit();
    return proto::LobbyEntry{.name = *name, .occupants = *occupants};
  }
};

template <typename O> struct serde::serializer<proto::LobbyList, O> {
  static auto serialize_to(const proto::LobbyList &msg, auto &ctx) {
    auto it = ctx.out();
    it = std::format_to(it, "LIST ");
    it = proto::wire::write_fixed2(
        static_cast<unsigned>(msg.entries.size()), it
    );
    for (const auto &entry : msg.entries) {
      it = proto::wire::write_sep(it);
      it = proto::wire::write_name(entry.name, it);
      it = proto::wire::write_sep(it);
      it = proto::wire::write_fixed2(entry.occupants, it);
    }
    return it;
  }
};

template <> struct serde::deserializer<proto::LobbyList> {
  static std::optional<proto::LobbyList> deserialize(auto &ctx) {
    serde::DeserializationTransaction tx(ctx);
    if (!ctx.assert_prefix("LIST") || !proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    auto count = proto::wire::read_fixed2(ctx);
    if (!count.has_value() || *count > proto::MAX_COUNT) {
      return std::nullopt;
    }
    proto::LobbyList out;
    for (auto _ : std::views::iota(0U, *count)) {
      if (!proto::wire::read_sep(ctx)) {
        return std::nullopt;
      }
      auto entry = serde::deserialize<proto::LobbyEntry>(ctx);
      if (!entry.has_value()) {
        return std::nullopt;
      }
      out.entries.push_back(*entry);
    }
    tx.commit();
    return out;
  }
};

template <typename O> struct serde::serializer<proto::Message, O> {
  static auto serialize_to(const proto::Message &msg, auto &ctx) {
    std::format_to(ctx.out(), "{} ", proto::MAGIC);
    std::visit([&](const auto &m) { return serde::serialize_to(m, ctx); }, msg);
    return std::format_to(ctx.out(), "\n");
  }
};

template <> struct serde::deserializer<proto::Message> {
  static std::optional<proto::Message> deserialize(auto &ctx) {
    serde::DeserializationTransaction tx(ctx);
    if (!ctx.assert_prefix(proto::MAGIC)) {
      return std::nullopt;
    }
    if (!proto::wire::read_sep(ctx)) {
      return std::nullopt;
    }
    std::string_view raw = ctx;
    if (raw.empty() || raw.back() != '\n') {
      return std::nullopt;
    }
    if (raw.size() > proto::MAX_LINE || raw.size() - 1 > proto::MAX_BODY_LEN) {
      return std::nullopt;
    }
    raw.remove_suffix(1);
    const auto try_body = [&]<typename T>() -> std::optional<proto::Message> {
      serde::DeserializationContext tmp(raw);
      auto body = serde::deserialize<T>(tmp);
      if (body.has_value() && tmp.empty()) {
        return proto::Message{*body};
      }
      return std::nullopt;
    };
    if (auto m = try_body.template operator()<proto::Login>()) {
      ctx.remove_prefix(ctx.size());
      tx.commit();
      return m;
    }
    if (auto m = try_body.template operator()<proto::Place>()) {
      ctx.remove_prefix(ctx.size());
      tx.commit();
      return m;
    }
    if (auto m = try_body.template operator()<proto::Move>()) {
      ctx.remove_prefix(ctx.size());
      tx.commit();
      return m;
    }
    if (auto m = try_body.template operator()<proto::RematchVote>()) {
      ctx.remove_prefix(ctx.size());
      tx.commit();
      return m;
    }
    if (auto m = try_body.template operator()<proto::Leave>()) {
      ctx.remove_prefix(ctx.size());
      tx.commit();
      return m;
    }
    if (auto m = try_body.template operator()<proto::ListLobbies>()) {
      // Bare LIST must not be shadowed by LIST-response prefix: LIST-response
      // requires "LIST SP ...", bare request is exactly "LIST".
      if (raw == "LIST") {
        ctx.remove_prefix(ctx.size());
        tx.commit();
        return m;
      }
    }
    if (auto m = try_body.template operator()<proto::Ack>()) {
      ctx.remove_prefix(ctx.size());
      tx.commit();
      return m;
    }
    if (auto m = try_body.template operator()<proto::Reject>()) {
      ctx.remove_prefix(ctx.size());
      tx.commit();
      return m;
    }
    if (auto m = try_body.template operator()<proto::GameStart>()) {
      ctx.remove_prefix(ctx.size());
      tx.commit();
      return m;
    }
    if (auto m = try_body.template operator()<proto::GameOver>()) {
      ctx.remove_prefix(ctx.size());
      tx.commit();
      return m;
    }
    if (auto m = try_body.template operator()<proto::LobbyStatus>()) {
      ctx.remove_prefix(ctx.size());
      tx.commit();
      return m;
    }
    if (auto m = try_body.template operator()<proto::LobbyList>()) {
      ctx.remove_prefix(ctx.size());
      tx.commit();
      return m;
    }
    return std::nullopt;
  }
};
