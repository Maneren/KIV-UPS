#include <hive/messages.h>

namespace hive {

MoveMessage::MoveMessage(Move move, Player player)
    : _move(move), _player(player) {}

bool MoveMessage::operator==(const MoveMessage &other) const {
  return _move == other._move && _player == other._player;
}

Move MoveMessage::move() const { return _move; }
Player MoveMessage::player() const { return _player; }

} // namespace hive
