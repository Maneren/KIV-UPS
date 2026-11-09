"""
Exceptions raised by the Hive domain layer.

All game-rule errors derive from :class:`RuntimeError`.
"""

from __future__ import annotations

__all__ = ["EmptyTileError", "HiveError", "IllegalMoveError"]


class HiveError(RuntimeError):
    """Base class for Hive domain errors."""


class EmptyTileError(HiveError):
    """An operation required a piece on a tile that has none."""


class IllegalMoveError(HiveError):
    """A move is not legal for the current game state."""
