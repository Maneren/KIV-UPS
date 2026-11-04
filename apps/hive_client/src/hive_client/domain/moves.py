"""Move value objects."""

from __future__ import annotations

from dataclasses import dataclass

from hive_client.domain.geometry import TilePointer
from hive_client.domain.pieces import PieceKind

__all__ = [
    "Move",
    "MovePiece",
    "PlacePiece",
    "make_move",
    "make_placement",
]


@dataclass(frozen=True)
class PlacePiece:
    """Place a piece of ``piece_kind`` at ``pos``."""

    pos: TilePointer
    piece_kind: PieceKind


@dataclass(frozen=True)
class MovePiece:
    """Move a piece of ``piece_kind`` from ``from_`` to ``to``."""

    from_: TilePointer
    to: TilePointer
    piece_kind: PieceKind


Move = PlacePiece | MovePiece


def make_move(from_: TilePointer, to: TilePointer, piece_kind: PieceKind) -> Move:
    """Create a ``MovePiece`` move."""
    return MovePiece(from_=from_, to=to, piece_kind=piece_kind)


def make_placement(pos: TilePointer, piece_kind: PieceKind) -> Move:
    """Create a ``PlacePiece`` move."""
    return PlacePiece(pos=pos, piece_kind=piece_kind)
