"""Players, piece kinds, and piece supply rules."""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum

__all__ = [
    "DEFAULT_PIECES_PER_PLAYER",
    "PIECE_KINDS",
    "Piece",
    "PieceKind",
    "Player",
    "make_initial_supply",
]


class Player(Enum):
    """Player colour: White moves first, Black second."""

    Black = 0
    White = 1

    def other(self) -> Player:
        """Return the opposing player."""
        return Player.Black if self is Player.White else Player.White


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

DEFAULT_PIECES_PER_PLAYER: dict[PieceKind, int] = {
    PieceKind.Queen: 1,
    PieceKind.Spider: 2,
    PieceKind.Beetle: 2,
    PieceKind.Grasshopper: 2,
    PieceKind.Ant: 2,
}


def make_initial_supply() -> dict[PieceKind, int]:
    """Return a fresh per-player supply of unplaced pieces."""
    return dict(DEFAULT_PIECES_PER_PLAYER)


@dataclass(frozen=True)
class Piece:
    """A piece on the board: kind plus owning player."""

    kind: PieceKind
    owner: Player

    def display(self) -> str:
        """Return the uppercase kind for White, lowercase for Black."""
        name = self.kind.name
        if self.owner is Player.White:
            return name.upper()
        return name.lower()
