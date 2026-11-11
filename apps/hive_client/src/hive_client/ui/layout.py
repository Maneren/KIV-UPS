"""Pixel layout for the Hive window (no drawing code)."""

from __future__ import annotations

from pyray import Rectangle

from hive_client.domain.pieces import PIECE_KINDS

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
    "rect_center",
]

AVAILABLE_X_OFFSET = 20.0
AVAILABLE_Y_OFFSET = 20.0
PALETTE_X_OFFSET = 20.0
PALETTE_Y_OFFSET = 130.0
PALETTE_TILE_SIZE = 48.0
PALETTE_ROW_HEIGHT = 56.0
TUTORIAL_PANEL_WIDTH = 400.0
TUTORIAL_X_OFFSET = 16.0
TUTORIAL_Y_OFFSET = 100.0


PALETTE_BOUNDS = Rectangle(
    PALETTE_X_OFFSET,
    PALETTE_Y_OFFSET,
    PALETTE_TILE_SIZE,
    PALETTE_ROW_HEIGHT * len(PIECE_KINDS),
)


PALETTE_PIECES = tuple(
    (
        kind,
        Rectangle(
            PALETTE_X_OFFSET,
            PALETTE_Y_OFFSET + index * PALETTE_ROW_HEIGHT,
            PALETTE_TILE_SIZE,
            PALETTE_TILE_SIZE,
        ),
    )
    for index, kind in enumerate(PIECE_KINDS)
)


def rect_center(rect: Rectangle) -> tuple[float, float]:
    """Return the centre of ``rect``."""
    return (rect.x + rect.width / 2, rect.y + rect.height / 2)
