"""
Hex-grid geometry primitives.

This module is deliberately independent of any rendering or I/O concerns: it
only knows about axial coordinates and the six hex directions.
"""

from __future__ import annotations

from collections.abc import Iterator
from dataclasses import dataclass
from typing import overload

__all__ = [
    "DIRECTIONS",
    "HEX_SIDES",
    "Direction",
    "TilePointer",
    "neighboring_cells",
    "rotate_left",
    "rotate_right",
]

HEX_SIDES = 6


@dataclass(frozen=True)
class Direction:
    """Axial offset between two neighbouring tiles."""

    p: int
    q: int


DIRECTIONS: tuple[Direction, ...] = (
    Direction(1, 0),
    Direction(0, 1),
    Direction(-1, 1),
    Direction(-1, 0),
    Direction(0, -1),
    Direction(1, -1),
)


@dataclass(frozen=True)
class TilePointer:
    """Axial hex coordinate ``(p, q)``."""

    p: int
    q: int

    def __hash__(self) -> int:
        """Hash by XOR of coordinates."""
        return hash(self.p) ^ hash(self.q)

    def __str__(self) -> str:
        """Return coordinate as ``(p, q)``."""
        return f"({self.p}, {self.q})"

    def __add__(self, other: Direction) -> TilePointer:
        """Add a direction to a tile pointer."""
        return TilePointer(self.p + other.p, self.q + other.q)

    @overload
    def __sub__(self, other: Direction) -> TilePointer: ...

    @overload
    def __sub__(self, other: TilePointer) -> Direction: ...

    def __sub__(self, other: Direction | TilePointer) -> Direction | TilePointer:
        """Subtract a direction or another tile from a tile pointer."""
        return TilePointer(self.p - other.p, self.q - other.q)


def rotate_left(direction: Direction) -> Direction:
    """Rotate a direction 60 degrees counter-clockwise."""
    return Direction(direction.p + direction.q, -direction.p)


def rotate_right(direction: Direction) -> Direction:
    """Rotate a direction 60 degrees clockwise."""
    return Direction(-direction.q, direction.p + direction.q)


def neighboring_cells(ptr: TilePointer) -> Iterator[TilePointer]:
    """Yield the six neighboring hex cells of ``ptr``."""
    for direction in DIRECTIONS:
        yield ptr + direction
