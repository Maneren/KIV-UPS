#include <proto/message.h>

#include <charconv>
#include <format>
#include <utility>

namespace proto {
namespace {

constexpr bool is_name_char(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') || c == '_' || c == '-';
}

bool valid_name(std::string_view name, std::size_t max_len) {
  if (name.empty() || name.size() > max_len) {
    return false;
  }
  for (char c : name) {
    if (!is_name_char(c)) {
      return false;
    }
  }
  return true;
}

char piece_to_char(hive::PieceKind piece) {
  switch (piece) {
  case hive::PieceKind::Queen:
    return 'Q';
  case hive::PieceKind::Spider:
    return 'S';
  case hive::PieceKind::Beetle:
    return 'B';
  case hive::PieceKind::Grasshopper:
    return 'G';
  case hive::PieceKind::Ant:
    return 'A';
  }
}

char player_to_char(hive::Player player) {
  switch (player) {
  case hive::Player::White:
    return 'W';
  case hive::Player::Black:
    return 'B';
  }
}

std::string_view reject_to_str(RejectCode code) {
  switch (code) {
  case RejectCode::BadSyntax:
    return "BAD_SYNTAX";
  case RejectCode::BadNick:
    return "BAD_NAME";
  case RejectCode::NotYourTurn:
    return "NOT_YOUR_TURN";
  case RejectCode::IllegalMove:
    return "ILLEGAL_MOVE";
  case RejectCode::BadState:
    return "BAD_STATE";
  }
}

std::string_view reason_to_str(EndReason reason) {
  switch (reason) {
  case EndReason::Surround:
    return "SURROUND";
  case EndReason::Forfeit:
    return "FORFEIT";
  }
}

std::string_view state_to_str(LobbyState state) {
  switch (state) {
  case LobbyState::Waiting:
    return "WAITING";
  case LobbyState::Queued:
    return "QUEUED";
  case LobbyState::Promoted:
    return "PROMOTED";
  case LobbyState::RematchWait:
    return "REMATCH_WAIT";
  }
}

std::optional<std::string> body_of(const Login &msg) {
  if (!valid_name(msg.nick, kMaxNickLen) ||
      !valid_name(msg.lobby, kMaxLobbyLen)) {
    return std::nullopt;
  }
  return std::format(
      "LOGIN {:02}{} {:02}{}",
      msg.nick.size(),
      msg.nick,
      msg.lobby.size(),
      msg.lobby
  );
}

std::optional<std::string> body_of(const Place &msg) {
  return std::format(
      "PLACE {} {} {}", msg.pos.p, msg.pos.q, piece_to_char(msg.piece)
  );
}

std::optional<std::string> body_of(const Move &msg) {
  return std::format(
      "MOVE {} {} {} {} {}",
      msg.from.p,
      msg.from.q,
      msg.to.p,
      msg.to.q,
      piece_to_char(msg.piece)
  );
}

std::optional<std::string> body_of(const RematchVote &) { return "REMATCH"; }

std::optional<std::string> body_of(const Leave &) { return "LEAVE"; }

std::optional<std::string> body_of(const ListLobbies &) { return "LIST"; }

std::optional<std::string> body_of(const Ack &) { return "ACK"; }

std::optional<std::string> body_of(const Reject &msg) {
  return std::format("REJECT {}", reject_to_str(msg.code));
}

std::optional<std::string> body_of(const GameStart &msg) {
  if (!valid_name(msg.opponent, kMaxNickLen)) {
    return std::nullopt;
  }
  return std::format(
      "START {} {:02}{}",
      player_to_char(msg.color),
      msg.opponent.size(),
      msg.opponent
  );
}

std::optional<std::string> body_of(const GameOver &msg) {
  const char winner =
      msg.winner.has_value() ? player_to_char(*msg.winner) : '-';
  return std::format("OVER {} {}", winner, reason_to_str(msg.reason));
}

std::optional<std::string> body_of(const LobbyStatus &msg) {
  if (msg.state == LobbyState::Queued) {
    if (!msg.position.has_value() || *msg.position < 1 ||
        *msg.position > kMaxListEntries) {
      return std::nullopt;
    }
    return std::format(
        "LOBBY {} {:02}", state_to_str(msg.state), *msg.position
    );
  }
  if (msg.position.has_value()) {
    return std::nullopt;
  }
  return std::format("LOBBY {}", state_to_str(msg.state));
}

std::optional<std::string> body_of(const LobbyList &msg) {
  if (msg.entries.size() > kMaxListEntries) {
    return std::nullopt;
  }
  std::string body = std::format("LIST {:02}", msg.entries.size());
  for (const auto &entry : msg.entries) {
    if (!valid_name(entry.name, kMaxLobbyLen) ||
        entry.occupants > kMaxListEntries) {
      return std::nullopt;
    }
    std::format_to(
        std::back_inserter(body),
        " {:02}{} {:02}",
        entry.name.size(),
        entry.name,
        entry.occupants
    );
  }
  return body;
}

// Strict cursor: failed parses consume nothing.
class Cursor {
  std::string_view rest_;

public:
  explicit Cursor(std::string_view data) : rest_(data) {}

  [[nodiscard]] bool empty() const { return rest_.empty(); }

  bool literal(std::string_view text) {
    if (!rest_.starts_with(text)) {
      return false;
    }
    rest_.remove_prefix(text.size());
    return true;
  }

  bool space() { return literal(" "); }

  std::optional<unsigned> digits(std::size_t width) {
    if (rest_.size() < width) {
      return std::nullopt;
    }
    unsigned value = 0;
    for (std::size_t i = 0; i < width; ++i) {
      const char c = rest_[i];
      if (c < '0' || c > '9') {
        return std::nullopt;
      }
      value = value * 10 + static_cast<unsigned>(c - '0');
    }
    rest_.remove_prefix(width);
    return value;
  }

  std::optional<std::string> name(std::size_t max_len) {
    const auto len = digits(2);
    if (!len.has_value() || *len == 0 || *len > max_len) {
      return std::nullopt;
    }
    if (rest_.size() < *len) {
      return std::nullopt;
    }
    const std::string_view view = rest_.substr(0, *len);
    for (char c : view) {
      if (!is_name_char(c)) {
        return std::nullopt;
      }
    }
    rest_.remove_prefix(*len);
    return std::string(view);
  }

  std::optional<hive::Coordinate> coordinate() {
    const std::size_t end = rest_.find(' ');
    const std::string_view token =
        end == std::string_view::npos ? rest_ : rest_.substr(0, end);
    if (token.empty()) {
      return std::nullopt;
    }
    hive::Coordinate value = 0;
    const auto [ptr, ec] =
        std::from_chars(token.data(), token.data() + token.size(), value);
    if (ec != std::errc{} || ptr != token.data() + token.size()) {
      return std::nullopt;
    }
    rest_.remove_prefix(token.size());
    return value;
  }

  std::optional<hive::PieceKind> piece() {
    if (rest_.empty()) {
      return std::nullopt;
    }
    hive::PieceKind kind;
    switch (rest_.front()) {
    case 'Q':
      kind = hive::PieceKind::Queen;
      break;
    case 'S':
      kind = hive::PieceKind::Spider;
      break;
    case 'B':
      kind = hive::PieceKind::Beetle;
      break;
    case 'G':
      kind = hive::PieceKind::Grasshopper;
      break;
    case 'A':
      kind = hive::PieceKind::Ant;
      break;
    default:
      return std::nullopt;
    }
    rest_.remove_prefix(1);
    return kind;
  }

  std::optional<hive::Player> color() {
    if (rest_.empty()) {
      return std::nullopt;
    }
    hive::Player player;
    switch (rest_.front()) {
    case 'W':
      player = hive::Player::White;
      break;
    case 'B':
      player = hive::Player::Black;
      break;
    default:
      return std::nullopt;
    }
    rest_.remove_prefix(1);
    return player;
  }

  // OVER winner: 'W'/'B' or '-' (none).
  std::optional<std::optional<hive::Player>> winner() {
    if (rest_.starts_with("-")) {
      rest_.remove_prefix(1);
      return std::optional<hive::Player>{std::nullopt};
    }
    auto player = color();
    if (!player.has_value()) {
      return std::nullopt;
    }
    return std::optional<hive::Player>{*player};
  }

  // Literal match; the caller rejects trailing garbage (space/end check).
  std::optional<RejectCode> reject_code() {
    constexpr std::pair<std::string_view, RejectCode> codes[] = {
        {"BAD_SYNTAX", RejectCode::BadSyntax},
        {"BAD_NAME", RejectCode::BadNick},
        {"NOT_YOUR_TURN", RejectCode::NotYourTurn},
        {"ILLEGAL_MOVE", RejectCode::IllegalMove},
        {"BAD_STATE", RejectCode::BadState},
    };
    for (const auto &[text, code] : codes) {
      if (rest_.starts_with(text)) {
        rest_.remove_prefix(text.size());
        return code;
      }
    }
    return std::nullopt;
  }

  std::optional<EndReason> end_reason() {
    constexpr std::pair<std::string_view, EndReason> reasons[] = {
        {"SURROUND", EndReason::Surround},
        {"FORFEIT", EndReason::Forfeit},
    };
    for (const auto &[text, reason] : reasons) {
      if (rest_.starts_with(text)) {
        rest_.remove_prefix(text.size());
        return reason;
      }
    }
    return std::nullopt;
  }

  std::optional<LobbyState> lobby_state() {
    constexpr std::pair<std::string_view, LobbyState> states[] = {
        {"WAITING", LobbyState::Waiting},
        {"QUEUED", LobbyState::Queued},
        {"PROMOTED", LobbyState::Promoted},
        {"REMATCH_WAIT", LobbyState::RematchWait},
    };
    for (const auto &[text, state] : states) {
      if (rest_.starts_with(text)) {
        rest_.remove_prefix(text.size());
        return state;
      }
    }
    return std::nullopt;
  }
};

std::optional<Message> decode_login(Cursor &cur) {
  if (!cur.space()) {
    return std::nullopt;
  }
  auto nick = cur.name(kMaxNickLen);
  if (!nick.has_value() || !cur.space()) {
    return std::nullopt;
  }
  auto lobby = cur.name(kMaxLobbyLen);
  if (!lobby.has_value() || !cur.empty()) {
    return std::nullopt;
  }
  return Message{Login{.nick = *nick, .lobby = *lobby}};
}

std::optional<Message> decode_place(Cursor &cur) {
  if (!cur.space()) {
    return std::nullopt;
  }
  auto p = cur.coordinate();
  if (!p.has_value() || !cur.space()) {
    return std::nullopt;
  }
  auto q = cur.coordinate();
  if (!q.has_value() || !cur.space()) {
    return std::nullopt;
  }
  auto piece = cur.piece();
  if (!piece.has_value() || !cur.empty()) {
    return std::nullopt;
  }
  return Message{Place{.pos = {.p = *p, .q = *q}, .piece = *piece}};
}

std::optional<Message> decode_move(Cursor &cur) {
  if (!cur.space()) {
    return std::nullopt;
  }
  auto fp = cur.coordinate();
  if (!fp.has_value() || !cur.space()) {
    return std::nullopt;
  }
  auto fq = cur.coordinate();
  if (!fq.has_value() || !cur.space()) {
    return std::nullopt;
  }
  auto tp = cur.coordinate();
  if (!tp.has_value() || !cur.space()) {
    return std::nullopt;
  }
  auto tq = cur.coordinate();
  if (!tq.has_value() || !cur.space()) {
    return std::nullopt;
  }
  auto piece = cur.piece();
  if (!piece.has_value() || !cur.empty()) {
    return std::nullopt;
  }
  return Message{Move{
      .from = {.p = *fp, .q = *fq}, .to = {.p = *tp, .q = *tq}, .piece = *piece
  }};
}

std::optional<Message> decode_list(Cursor &cur) {
  if (cur.empty()) {
    return Message{ListLobbies{}};
  }
  if (!cur.space()) {
    return std::nullopt;
  }
  auto count = cur.digits(2);
  if (!count.has_value() || *count > kMaxListEntries) {
    return std::nullopt;
  }
  LobbyList list;
  for (unsigned i = 0; i < *count; ++i) {
    if (!cur.space()) {
      return std::nullopt;
    }
    auto name = cur.name(kMaxLobbyLen);
    if (!name.has_value() || !cur.space()) {
      return std::nullopt;
    }
    auto occupants = cur.digits(2);
    if (!occupants.has_value()) {
      return std::nullopt;
    }
    list.entries.push_back(LobbyEntry{.name = *name, .occupants = *occupants});
  }
  if (!cur.empty()) {
    return std::nullopt;
  }
  return Message{list};
}

std::optional<Message> decode_loby(Cursor &cur) {
  if (!cur.space()) {
    return std::nullopt;
  }
  auto state = cur.lobby_state();
  if (!state.has_value()) {
    return std::nullopt;
  }
  if (*state == LobbyState::Queued) {
    if (!cur.space()) {
      return std::nullopt;
    }
    auto pos = cur.digits(2);
    if (!pos.has_value() || *pos < 1 || !cur.empty()) {
      return std::nullopt;
    }
    return Message{LobbyStatus{.state = *state, .position = *pos}};
  }
  if (!cur.empty()) {
    return std::nullopt;
  }
  return Message{LobbyStatus{.state = *state, .position = std::nullopt}};
}

std::optional<Message> decode_body(std::string_view body) {
  Cursor cur(body);
  if (cur.literal("LOGIN")) {
    return decode_login(cur);
  }
  if (cur.literal("PLACE")) {
    return decode_place(cur);
  }
  if (cur.literal("MOVE")) {
    return decode_move(cur);
  }
  if (cur.literal("REMATCH")) {
    return cur.empty() ? std::optional<Message>{Message{RematchVote{}}}
                       : std::nullopt;
  }
  if (cur.literal("LEAVE")) {
    return cur.empty() ? std::optional<Message>{Message{Leave{}}}
                       : std::nullopt;
  }
  if (cur.literal("LIST")) {
    return decode_list(cur);
  }
  if (cur.literal("ACK")) {
    return cur.empty() ? std::optional<Message>{Message{Ack{}}} : std::nullopt;
  }
  if (cur.literal("REJECT")) {
    if (!cur.space()) {
      return std::nullopt;
    }
    auto code = cur.reject_code();
    if (!code.has_value() || !cur.empty()) {
      return std::nullopt;
    }
    return Message{Reject{.code = *code}};
  }
  if (cur.literal("START")) {
    if (!cur.space()) {
      return std::nullopt;
    }
    auto color = cur.color();
    if (!color.has_value() || !cur.space()) {
      return std::nullopt;
    }
    auto opponent = cur.name(kMaxNickLen);
    if (!opponent.has_value() || !cur.empty()) {
      return std::nullopt;
    }
    return Message{GameStart{.color = *color, .opponent = *opponent}};
  }
  if (cur.literal("OVER")) {
    if (!cur.space()) {
      return std::nullopt;
    }
    auto winner = cur.winner();
    if (!winner.has_value() || !cur.space()) {
      return std::nullopt;
    }
    auto reason = cur.end_reason();
    if (!reason.has_value() || !cur.empty()) {
      return std::nullopt;
    }
    return Message{GameOver{.winner = *winner, .reason = *reason}};
  }
  if (cur.literal("LOBBY")) {
    return decode_loby(cur);
  }
  return std::nullopt;
}

} // namespace

std::optional<std::string> encode(const Message &msg) {
  auto body = std::visit([](const auto &m) { return body_of(m); }, msg);
  if (!body.has_value() || body->size() > kMaxBodyLen) {
    return std::nullopt;
  }
  return std::format("{}{}\n", kMagic, *body);
}

std::optional<Message> decode(std::string_view frame) {
  if (frame.size() < kMagic.size() + 3 + 1) {
    return std::nullopt;
  }
  if (!frame.starts_with(kMagic)) {
    return std::nullopt;
  }
  if (frame.back() != '\n') {
    return std::nullopt;
  }
  const std::string_view body =
      frame.substr(kMagic.size(), frame.size() - kMagic.size() - 1);
  if (body.size() < 3 || body.size() > kMaxBodyLen) {
    return std::nullopt;
  }
  return decode_body(body);
}

TakeResult take_frame(std::string &buffer) {
  const std::size_t nl = buffer.find('\n');
  if (nl == std::string::npos) {
    if (buffer.size() > kMaxLine) {
      buffer.clear();
      return {TakeStatus::Error, {}};
    }
    return {TakeStatus::Incomplete, {}};
  }
  const std::size_t total = nl + 1;
  std::string frame = buffer.substr(0, total);
  buffer.erase(0, total);
  if (total > kMaxLine || !frame.starts_with(kMagic)) {
    return {TakeStatus::Error, {}};
  }
  return {TakeStatus::Ok, std::move(frame)};
}

} // namespace proto
