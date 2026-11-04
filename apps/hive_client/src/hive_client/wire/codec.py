"""
Encoding and decoding for the Hive text protocol.

The codec is pure: it turns strings/token lists into message dataclasses and
back.  It does not perform any I/O.
"""

from __future__ import annotations

from hive_client.domain.geometry import TilePointer
from hive_client.domain.moves import MovePiece, PlacePiece
from hive_client.domain.pieces import PieceKind
from hive_client.wire.messages import (
    MAGIC,
    Ack,
    Command,
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
    Start,
)

__all__ = [
    "MAGIC",
    "PIECE_KIND_TO_LETTER",
    "PIECE_LETTER_TO_KIND",
    "move_from_wire",
    "move_to_wire",
    "parse_command",
    "piece_kind_from_letter",
    "piece_letter_from_kind",
    "place_from_wire",
    "place_to_wire",
    "serialize",
]

PIECE_LETTER_TO_KIND: dict[str, PieceKind] = {
    "Q": PieceKind.Queen,
    "S": PieceKind.Spider,
    "B": PieceKind.Beetle,
    "G": PieceKind.Grasshopper,
    "A": PieceKind.Ant,
}

PIECE_KIND_TO_LETTER: dict[PieceKind, str] = {
    kind: letter for letter, kind in PIECE_LETTER_TO_KIND.items()
}


def piece_kind_from_letter(letter: str) -> PieceKind:
    """Convert a wire-protocol piece letter to a PieceKind."""
    try:
        return PIECE_LETTER_TO_KIND[letter.upper()]
    except KeyError as exc:
        raise ValueError(f"unknown piece letter: {letter!r}") from exc


def piece_letter_from_kind(kind: PieceKind) -> str:
    """Convert a PieceKind to its wire-protocol letter."""
    try:
        return PIECE_KIND_TO_LETTER[kind]
    except KeyError as exc:
        raise ValueError(f"unknown piece kind: {kind!r}") from exc


def place_from_wire(p: int, q: int, piece_letter: str) -> PlacePiece:
    """Create a PlacePiece from wire-protocol coordinates and piece letter."""
    return PlacePiece(
        pos=TilePointer(p, q),
        piece_kind=piece_kind_from_letter(piece_letter),
    )


def place_to_wire(place: PlacePiece) -> tuple[int, int, str]:
    """Convert a PlacePiece to wire-protocol coordinates and piece letter."""
    return (
        place.pos.p,
        place.pos.q,
        piece_letter_from_kind(place.piece_kind),
    )


def move_from_wire(
    from_p: int,
    from_q: int,
    to_p: int,
    to_q: int,
    piece_letter: str,
) -> MovePiece:
    """Create a MovePiece from wire-protocol coordinates and piece letter."""
    return MovePiece(
        from_=TilePointer(from_p, from_q),
        to=TilePointer(to_p, to_q),
        piece_kind=piece_kind_from_letter(piece_letter),
    )


def move_to_wire(move: MovePiece) -> tuple[int, int, int, int, str]:
    """Convert a MovePiece to wire-protocol coordinates and piece letter."""
    return (
        move.from_.p,
        move.from_.q,
        move.to.p,
        move.to.q,
        piece_letter_from_kind(move.piece_kind),
    )


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
    return Place(p=p, q=q, piece=piece_kind_from_letter(rest[2]))


def _parse_move(rest: list[str]) -> Move:
    return Move(
        from_p=int(rest[0]),
        from_q=int(rest[1]),
        to_p=int(rest[2]),
        to_q=int(rest[3]),
        piece=piece_kind_from_letter(rest[4]),
    )


def _parse_lobbies(rest: list[str]) -> Lobbies:
    count = int(rest[0])
    entries: list[tuple[str, int]] = []
    i = 1
    for _ in range(count):
        lobby, i = _read_nick_or_lobby(rest, i)
        entries.append((lobby, int(rest[i])))
        i += 1
    return Lobbies(entries=entries)


def parse_command(tokens: list[str]) -> IncomingMessage:
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


def serialize(cmd: Command) -> str:
    """Serialize a command dataclass into its wire representation."""
    match cmd:
        case Login():
            return _serialize_login(cmd)
        case Place():
            return _serialize_place(cmd)
        case Move():
            return _serialize_move(cmd)
        case Rematch():
            return f"{MAGIC} REMATCH"
        case Leave():
            return f"{MAGIC} LEAVE"
        case ListRequest():
            return f"{MAGIC} LIST"
    raise TypeError(f"cannot serialize {cmd!r}")


def _serialize_login(cmd: Login) -> str:
    return (
        f"{MAGIC} LOGIN {len(cmd.nick):02d}{cmd.nick} {len(cmd.lobby):02d}{cmd.lobby}"
    )


def _serialize_place(cmd: Place) -> str:
    letter = piece_letter_from_kind(cmd.piece)
    return f"{MAGIC} PLACE {cmd.p} {cmd.q} {letter}"


def _serialize_move(cmd: Move) -> str:
    letter = piece_letter_from_kind(cmd.piece)
    return f"{MAGIC} MOVE {cmd.from_p} {cmd.from_q} {cmd.to_p} {cmd.to_q} {letter}"
