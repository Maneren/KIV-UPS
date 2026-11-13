"""Game outcome values and the opening tile."""

from __future__ import annotations

from enum import IntEnum

from hive_client.domain.geometry import TilePointer

__all__ = ["ORIGIN", "GameResult"]

#: The first piece of the game is always placed at the board centre.
ORIGIN = TilePointer(0, 0)


class GameResult(IntEnum):
    """Outcome of a Hive game."""

    ONGOING = 0
    WHITE_WINS = 1
    BLACK_WINS = 2
    DRAW = 3
