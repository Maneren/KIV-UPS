"""Axial hex coordinates."""

from __future__ import annotations

from typing import NamedTuple

__all__ = ["TilePointer"]


class TilePointer(NamedTuple):
    """Axial hex coordinate ``(p, q)``."""

    p: int
    q: int

    def __hash__(self) -> int:
        """Hash by XOR of coordinates."""
        return hash(self.p) ^ hash(self.q)

    def __str__(self) -> str:
        """Return coordinate as ``(p, q)``."""
        return f"({self.p}, {self.q})"
