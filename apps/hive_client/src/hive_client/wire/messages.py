"""
Wire-protocol message value objects.

These mirror ``src/hive/PROTOCOL.md``.  They contain no parsing, socket, or
game logic; see :mod:`hive_client.wire.codec` and
:mod:`hive_client.wire.transport`.
"""

from __future__ import annotations

from dataclasses import dataclass, field

from hive_client.domain.pieces import PieceKind

__all__ = [
    "MAGIC",
    "Ack",
    "Command",
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
]

MAGIC = "HIVE"


@dataclass(frozen=True)
class Login:
    """Client -> server: join or create a lobby with a nick."""

    nick: str
    lobby: str


@dataclass(frozen=True)
class Place:
    """Client -> server: place a piece at (p, q)."""

    p: int
    q: int
    piece: PieceKind


@dataclass(frozen=True)
class Move:
    """Client -> server: move a piece from one cell to another."""

    from_p: int
    from_q: int
    to_p: int
    to_q: int
    piece: PieceKind


@dataclass(frozen=True)
class Rematch:
    """Client -> server: vote to play again."""


@dataclass(frozen=True)
class Leave:
    """Client -> server: leave the lobby or game."""


@dataclass(frozen=True)
class ListRequest:
    """Client -> server: request the lobby listing."""


@dataclass(frozen=True)
class Ack:
    """Server -> client: generic accept."""


@dataclass(frozen=True)
class Reject:
    """Server -> client: command was rejected, with a reason code."""

    code: str  # BAD_SYNTAX / BAD_NAME / NOT_YOUR_TURN / ILLEGAL_MOVE / BAD_STATE


@dataclass(frozen=True)
class Start:
    """Server -> client: game start, your colour and opponent nick."""

    color: str  # "W" / "B"
    nick: str  # opponent nick


@dataclass(frozen=True)
class Over:
    """Server -> client: game end with colour and reason."""

    color: str  # "W" / "B"
    reason: str  # SURROUND / FORFEIT


@dataclass(frozen=True)
class LobbyStatus:
    """Server -> client: lobby state transition."""

    state: str  # WAITING / QUEUED / PROMOTED / REMATCH_WAIT
    count: int | None = None


@dataclass(frozen=True)
class Lobbies:
    """Server -> client: list of lobbies with occupant counts."""

    entries: list[tuple[str, int]] = field(default_factory=list)


Command = Login | Place | Move | Rematch | Leave | ListRequest

# Server -> client messages (not sent by the client).
ServerMessage = Ack | Reject | Start | Over | LobbyStatus | Lobbies

IncomingMessage = Command | ServerMessage
