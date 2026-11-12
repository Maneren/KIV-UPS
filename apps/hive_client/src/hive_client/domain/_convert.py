"""Conversions between Python value objects and native move records."""

from __future__ import annotations

from hive_client.domain.geometry import TilePointer
from hive_client.domain.moves import Move, MovePiece, PlacePiece
from hive_client.domain.pieces import Piece, PieceKind, Player

__all__ = [
    "MOVE",
    "PLACE",
    "move_from_record",
    "move_to_record",
    "piece_from_record",
    "tile_from_record",
]

PLACE = 0
MOVE = 1

MoveRecord = tuple[int, int, int, int, int, int]


def move_to_record(move: Move) -> MoveRecord:
    """Encode a Python move as ``(type, from_p, from_q, to_p, to_q, kind)``."""
    if isinstance(move, PlacePiece):
        return (
            PLACE,
            0,
            0,
            move.pos.p,
            move.pos.q,
            move.piece_kind.value,
        )
    return (
        MOVE,
        move.from_.p,
        move.from_.q,
        move.to.p,
        move.to.q,
        move.piece_kind.value,
    )


def move_from_record(record: MoveRecord) -> Move:
    """Decode a native move record into a Python move."""
    kind = PieceKind(record[5])
    if record[0] == PLACE:
        return PlacePiece(pos=TilePointer(record[3], record[4]), piece_kind=kind)
    return MovePiece(
        from_=TilePointer(record[1], record[2]),
        to=TilePointer(record[3], record[4]),
        piece_kind=kind,
    )


def piece_from_record(record: tuple[int, int]) -> Piece:
    """Decode a ``(piece_kind, owner)`` record into a :class:`Piece`."""
    return Piece(kind=PieceKind(record[0]), owner=Player(record[1]))


def tile_from_record(record: tuple[int, int]) -> TilePointer:
    """Decode a ``(p, q)`` record into a :class:`TilePointer`."""
    return TilePointer(record[0], record[1])
