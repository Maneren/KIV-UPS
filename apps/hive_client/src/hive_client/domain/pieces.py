"""Players, piece kinds, and pieces."""

from __future__ import annotations

from enum import Enum
from typing import NamedTuple

__all__ = ["PIECE_KINDS", "Piece", "PieceKind", "Player"]


class Player(Enum):
    """Player colour: White moves first, Black second."""

    Black = 0
    White = 1


class PieceKind(Enum):
    """Kind of Hive piece."""

    Queen = 0
    Spider = 1
    Beetle = 2
    Grasshopper = 3
    Ant = 4


#: Stable display/iteration order used by palettes and UI lists.
PIECE_KINDS: tuple[PieceKind, ...] = (
    PieceKind.Queen,
    PieceKind.Spider,
    PieceKind.Beetle,
    PieceKind.Grasshopper,
    PieceKind.Ant,
)


class Piece(NamedTuple):
    """A piece on the board: kind plus owning player."""

    kind: PieceKind
    owner: Player
