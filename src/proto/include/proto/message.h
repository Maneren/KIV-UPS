#pragma once

#include <cstdint>
#include <hive/types.h>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace proto {

// Wire limits, see src/hive/PROTOCOL.md.
constexpr std::string_view kMagic = "HIVE ";
constexpr std::size_t kMaxNickLen = 16;
constexpr std::size_t kMaxLobbyLen = 32;
constexpr std::size_t kMaxBodyLen = 9999;
constexpr std::size_t kMaxLine = 16 * 1024;
constexpr unsigned kMaxListEntries = 99;

enum class RejectCode : std::uint8_t {
  BadSyntax,   // BADS
  BadNick,     // BADN
  NotYourTurn, // NOTN
  IllegalMove, // ILLG
  BadState,    // STAT
};

enum class EndReason : std::uint8_t {
  Surround, // SURN
  Forfeit,  // FORF
};

enum class LobbyState : std::uint8_t {
  Waiting,     // WAIT
  Queued,      // QUEU
  Promoted,    // PROM
  RematchWait, // RMWT
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
  // nullopt encodes as '-'; reserved in beta, accepted on decode.
  std::optional<hive::Player> winner;
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

// Encode one message; nullopt when it violates protocol limits.
std::optional<std::string> encode(const Message &msg);

// Decode one wire message; nullopt on violation, never throws.
std::optional<Message> decode(std::string_view frame);

enum class TakeStatus { Ok, Incomplete, Error };

struct TakeResult {
  TakeStatus status;
  std::string frame; // valid only when status == Ok
};

// Take the first line; Error drops it.
TakeResult take_frame(std::string &buffer);

} // namespace proto
