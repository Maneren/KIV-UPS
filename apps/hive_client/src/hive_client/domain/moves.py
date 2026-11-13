"""Move records shared with the native engine."""

from __future__ import annotations

from typing import NamedTuple

from hive_client.domain.geometry import TilePointer
from hive_client.domain.pieces import PieceKind

__all__ = ["MOVE", "PLACE", "Move"]

PLACE = 0
MOVE = 1


class Move(NamedTuple):
    """Native move record ``(type, from_p, from_q, to_p, to_q, kind)``."""

    type: int
    from_p: int
    from_q: int
    to_p: int
    to_q: int
    piece_kind: int

    @classmethod
    def place(cls, pos: TilePointer, piece_kind: PieceKind) -> Move:
        """Create a placement record."""
        return cls(PLACE, 0, 0, pos.p, pos.q, piece_kind.value)

    @classmethod
    def move(
        cls,
        from_: TilePointer,
        to: TilePointer,
        piece_kind: PieceKind,
    ) -> Move:
        """Create a movement record."""
        return cls(MOVE, from_.p, from_.q, to.p, to.q, piece_kind.value)

    @property
    def is_placement(self) -> bool:
        """Return True for a placement rather than a movement."""
        return self.type == PLACE

    @property
    def from_tile(self) -> TilePointer:
        """Return the source tile (unused for placements)."""
        return TilePointer(self.from_p, self.from_q)

    @property
    def to_tile(self) -> TilePointer:
        """Return the destination tile."""
        return TilePointer(self.to_p, self.to_q)

    @property
    def kind(self) -> PieceKind:
        """Return the piece kind as an enum."""
        return PieceKind(self.piece_kind)
