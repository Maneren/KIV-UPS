"""
raylib GUI (backward-compatible facade).

The implementation is split across :mod:`hive_client.ui` and
:mod:`hive_client.app`; this module keeps the historical import path.
"""

from __future__ import annotations

from pyray import Rectangle, Vector2

from hive_client.domain.geometry import HEX_SIDES, TilePointer
from hive_client.domain.pieces import PieceKind
from hive_client.ui.app import HiveGUI, run
from hive_client.ui.geometry import (
    HEX_ANGLE_STEP,
    HEX_SIZE,
    HEX_X_FACTOR,
    HEX_Y_FACTOR,
)
from hive_client.ui.geometry import hex_to_pixel as _hex_to_pixel
from hive_client.ui.geometry import snap_to_pixel as _snap_to_pixel
from hive_client.ui.input import MouseEvent
from hive_client.ui.layout import (
    AVAILABLE_X_OFFSET,
    AVAILABLE_Y_OFFSET,
    PALETTE_ROW_HEIGHT,
    PALETTE_TILE_SIZE,
    PALETTE_X_OFFSET,
    PALETTE_Y_OFFSET,
    TUTORIAL_PANEL_WIDTH,
    TUTORIAL_X_OFFSET,
    TUTORIAL_Y_OFFSET,
    palette_tiles,
)
from hive_client.ui.renderer import HexRenderer
from hive_client.ui.theme import (
    OUTLINE_THICKNESS,
    OUTLINE_THICKNESS_SPECIAL,
    PIECE_COLORS,
    PIECE_LETTERS,
    PIECE_SELECT_KEYS,
    TEXT_FONT_SIZE,
    TUTORIAL_FONT_SIZE,
    TUTORIAL_LINES,
    TUTORIAL_TOGGLE_KEY,
    piece_letter,
)

__all__ = [
    "AVAILABLE_X_OFFSET",
    "AVAILABLE_Y_OFFSET",
    "HEX_ANGLE_STEP",
    "HEX_SIDES",
    "HEX_SIZE",
    "HEX_X_FACTOR",
    "HEX_Y_FACTOR",
    "OUTLINE_THICKNESS",
    "OUTLINE_THICKNESS_SPECIAL",
    "PALETTE_ROW_HEIGHT",
    "PALETTE_TILE_SIZE",
    "PALETTE_X_OFFSET",
    "PALETTE_Y_OFFSET",
    "PIECE_COLORS",
    "PIECE_LETTERS",
    "PIECE_SELECT_KEYS",
    "TEXT_FONT_SIZE",
    "TUTORIAL_FONT_SIZE",
    "TUTORIAL_LINES",
    "TUTORIAL_PANEL_WIDTH",
    "TUTORIAL_TOGGLE_KEY",
    "TUTORIAL_X_OFFSET",
    "TUTORIAL_Y_OFFSET",
    "HexRenderer",
    "HiveGUI",
    "MouseEvent",
    "hex_to_pixel",
    "piece_letter",
    "run",
    "snap_to_hex",
]


def hex_to_pixel(ptr: TilePointer, center: Vector2) -> Vector2:
    """Convert axial coordinates to a raylib ``Vector2`` (legacy helper)."""
    x, y = _hex_to_pixel(ptr, (center.x, center.y))
    return Vector2(x, y)


def snap_to_hex(mouse: Vector2, center: Vector2) -> Vector2:
    """Snap a raylib position to the nearest hex centre (legacy helper)."""
    x, y = _snap_to_pixel((mouse.x, mouse.y), (center.x, center.y))
    return Vector2(x, y)


def _palette_tiles() -> list[tuple[PieceKind, Rectangle]]:
    """Return legacy ``Rectangle`` palette entries."""
    return [
        (kind, Rectangle(rect.x, rect.y, rect.width, rect.height))
        for kind, rect in palette_tiles()
    ]
