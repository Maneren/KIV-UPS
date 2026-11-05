"""Pixel layout for the Hive window (no drawing code)."""

from __future__ import annotations

from dataclasses import dataclass

from hive_client.domain.pieces import PIECE_KINDS, PieceKind

__all__ = [
    "AVAILABLE_X_OFFSET",
    "AVAILABLE_Y_OFFSET",
    "PALETTE_ROW_HEIGHT",
    "PALETTE_TILE_SIZE",
    "PALETTE_X_OFFSET",
    "PALETTE_Y_OFFSET",
    "TUTORIAL_PANEL_WIDTH",
    "TUTORIAL_X_OFFSET",
    "TUTORIAL_Y_OFFSET",
    "Rect",
    "palette_tiles",
]

AVAILABLE_X_OFFSET = 20.0
AVAILABLE_Y_OFFSET = 20.0
PALETTE_X_OFFSET = 20.0
PALETTE_Y_OFFSET = 130.0
PALETTE_TILE_SIZE = 48.0
PALETTE_ROW_HEIGHT = 56.0
TUTORIAL_PANEL_WIDTH = 320.0
TUTORIAL_X_OFFSET = 16.0
TUTORIAL_Y_OFFSET = 100.0


@dataclass(frozen=True)
class Rect:
    """Axis-aligned rectangle in screen pixels."""

    x: float
    y: float
    width: float
    height: float

    def contains(self, point: tuple[float, float]) -> bool:
        """Return True if ``point`` lies inside this rectangle."""
        x, y = point
        return self.x <= x <= self.x + self.width and (
            self.y <= y <= self.y + self.height
        )

    @property
    def center(self) -> tuple[float, float]:
        """Return the rectangle's centre point."""
        return (self.x + self.width / 2.0, self.y + self.height / 2.0)


def palette_tiles() -> tuple[tuple[PieceKind, Rect], ...]:
    """Return ``(kind, rect)`` for each draggable palette tile."""
    return tuple(
        (
            kind,
            Rect(
                PALETTE_X_OFFSET,
                PALETTE_Y_OFFSET + index * PALETTE_ROW_HEIGHT,
                PALETTE_TILE_SIZE,
                PALETTE_TILE_SIZE,
            ),
        )
        for index, kind in enumerate(PIECE_KINDS)
    )
