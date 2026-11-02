r"""
TCP client for the Hive game protocol (see src/hive/PROTOCOL.md).

Every message is HIVE <command><newline>.  The client connects to a server,
sends commands, and parses the server's responses into small dataclasses.
"""

from __future__ import annotations

import socket
from dataclasses import dataclass

__all__ = [
    "Ack",
    "Command",
    "HiveClient",
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
    piece: str  # "Q" / "S" / "B" / "G" / "A"


@dataclass(frozen=True)
class Move:
    """Client -> server: move a piece from one cell to another."""

    from_p: int
    from_q: int
    to_p: int
    to_q: int
    piece: str


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

    entries: list[tuple[str, int]]  # (lobby_name, occupant_count)


Command = Login | Place | Move | Rematch | Leave | ListRequest


def _read_nick_or_lobby(tokens: list[str], index: int) -> tuple[str, int]:
    """Read a length-prefixed name field (2 digit length + chars)."""
    field = tokens[index]
    length = int(field[:2])
    name = field[2 : 2 + length]
    return name, index + 1


def _parse_login(rest: list[str]) -> Login:
    nick, i = _read_nick_or_lobby(rest, 0)
    lobby, _ = _read_nick_or_lobby(rest, i)
    return Login(nick=nick, lobby=lobby)


def _parse_place(rest: list[str]) -> Place:
    p, q = int(rest[0]), int(rest[1])
    return Place(p=p, q=q, piece=rest[2])


def _parse_move(rest: list[str]) -> Move:
    return Move(
        from_p=int(rest[0]),
        from_q=int(rest[1]),
        to_p=int(rest[2]),
        to_q=int(rest[3]),
        piece=rest[4],
    )


def parse_command(tokens: list[str]) -> object:
    """
    Parse a command line (without the HIVE magic) into a dataclass.

    The first token is the command name; the rest are its arguments.
    """
    if not tokens:
        raise ValueError("empty command")

    name = tokens[0]
    rest = tokens[1:]

    parsers = {
        "LOGIN": _parse_login,
        "PLACE": _parse_place,
        "MOVE": _parse_move,
        "REMATCH": lambda _: Rematch(),
        "LEAVE": lambda _: Leave(),
        "LIST": lambda _: ListRequest(),
        "ACK": lambda _: Ack(),
        "REJECT": lambda r: Reject(code=r[0]),
        "START": lambda r: Start(color=r[0], nick=r[1]),
        "OVER": lambda r: Over(color=r[0], reason=r[1]),
        "LOBBIES": _parse_lobbies,
    }

    if name in parsers:
        return parsers[name](rest)

    if name == "LOBBY":
        count = int(rest[1]) if len(rest) > 1 else None
        return LobbyStatus(state=rest[0], count=count)

    raise ValueError(f"unknown command: {name}")


def _parse_lobbies(rest: list[str]) -> Lobbies:
    count = int(rest[0])
    entries: list[tuple[str, int]] = []
    i = 1
    for _ in range(count):
        lobby, i = _read_nick_or_lobby(rest, i)
        entries.append((lobby, int(rest[i])))
        i += 1
    return Lobbies(entries=entries)


def serialize(cmd: Command) -> str:
    """Serialize a command dataclass into its wire representation."""
    if isinstance(cmd, Login):
        return _serialize_login(cmd)
    if isinstance(cmd, Place):
        return _serialize_place(cmd)
    if isinstance(cmd, Move):
        return _serialize_move(cmd)
    if isinstance(cmd, (Rematch, Leave, ListRequest, Ack)):
        return _serialize_simple(cmd)
    raise TypeError(f"cannot serialize {cmd!r}")


def _serialize_simple(cmd: Rematch | Leave | ListRequest | Ack) -> str:
    match cmd:
        case Rematch():
            return f"{MAGIC} REMATCH"
        case Leave():
            return f"{MAGIC} LEAVE"
        case ListRequest():
            return f"{MAGIC} LIST"
        case Ack():
            return f"{MAGIC} ACK"
    raise TypeError(f"cannot serialize {cmd!r}")


def _serialize_login(cmd: Login) -> str:
    return (
        f"{MAGIC} LOGIN {len(cmd.nick):02d}{cmd.nick} {len(cmd.lobby):02d}{cmd.lobby}"
    )


def _serialize_place(cmd: Place) -> str:
    return f"{MAGIC} PLACE {cmd.p} {cmd.q} {cmd.piece}"


def _serialize_move(cmd: Move) -> str:
    return f"{MAGIC} MOVE {cmd.from_p} {cmd.from_q} {cmd.to_p} {cmd.to_q} {cmd.piece}"


class HiveClient:
    """A TCP connection to a Hive server with buffered line-based I/O."""

    def __init__(self, host: str, port: int) -> None:
        """Create a client for ``host``:``port`` (connection is lazy)."""
        self._host = host
        self._port = port
        self._sock: socket.socket | None = None
        self._buffer = ""

    def connect(self, timeout: float = 5.0) -> None:
        """Open a TCP connection to the server."""
        self._sock = socket.create_connection((self._host, self._port), timeout)
        self._sock.settimeout(None)  # blocking after connect

    def close(self) -> None:
        """Close the TCP connection."""
        if self._sock is not None:
            self._sock.close()
            self._sock = None

    def send(self, cmd: Command) -> None:
        """Send a command to the server."""
        if self._sock is None:
            raise RuntimeError("not connected")
        line = serialize(cmd) + "\n"
        self._sock.sendall(line.encode("ascii"))

    def receive(self) -> object:
        """Block until one full message is available and parse it."""
        if self._sock is None:
            raise RuntimeError("not connected")

        while "\n" not in self._buffer:
            chunk = self._sock.recv(4096)
            if not chunk:
                raise ConnectionError("server closed the connection")
            self._buffer += chunk.decode("ascii")

        line, self._buffer = self._buffer.split("\n", 1)
        tokens = line.split(" ")
        if not tokens or tokens[0] != MAGIC:
            raise ValueError(f"bad magic: {tokens[0] if tokens else None!r}")
        return parse_command(tokens[1:])
