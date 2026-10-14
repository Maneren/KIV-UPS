#pragma once

#include "types.h"

namespace hive {

class Message {
public:
  Message() = default;
  Message(const Message &) = default;
  Message(Message &&) = default;
  Message &operator=(const Message &) = default;
  Message &operator=(Message &&) = default;

  virtual ~Message() = default;

  bool operator==(const Message &other) const = default;
};

class MoveMessage : public Message {
  Move _move;
  Player _player;

public:
  MoveMessage(Move move, Player player);

  bool operator==(const MoveMessage &other) const;

  [[nodiscard]] Move move() const;
  [[nodiscard]] Player player() const;
};

class OkMessage : public Message {
public:
  OkMessage() = default;

  bool operator==(const OkMessage & /*unused*/) const { return true; }
};

} // namespace hive
