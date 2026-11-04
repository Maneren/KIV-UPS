"""
TCP client for the Hive game protocol (backward-compatible facade).

Message types, the pure codec, and the socket transport now live in
:mod:`hive_client.wire`.
"""

from __future__ import annotations

from hive_client.wire import (
    MAGIC,
    PIECE_KIND_TO_LETTER,
    PIECE_LETTER_TO_KIND,
    Ack,
    Command,
    HiveClient,
    IncomingMessage,
    Leave,
    ListRequest,
    Lobbies,
    LobbyStatus,
    Login,
    Move,
    Over,
    Place,
    Reject,
    Rematch,
    ServerMessage,
    Start,
    move_from_wire,
    move_to_wire,
    parse_command,
    piece_kind_from_letter,
    piece_letter_from_kind,
    place_from_wire,
    place_to_wire,
    serialize,
)

__all__ = [
    "MAGIC",
    "PIECE_KIND_TO_LETTER",
    "PIECE_LETTER_TO_KIND",
    "Ack",
    "Command",
    "HiveClient",
    "IncomingMessage",
    "Leave",
    "ListRequest",
    "Lobbies",
    "LobbyStatus",
    "Login",
    "Move",
    "Over",
    "Place",
    "Reject",
    "Rematch",
    "ServerMessage",
    "Start",
    "move_from_wire",
    "move_to_wire",
    "parse_command",
    "piece_kind_from_letter",
    "piece_letter_from_kind",
    "place_from_wire",
    "place_to_wire",
    "serialize",
]
