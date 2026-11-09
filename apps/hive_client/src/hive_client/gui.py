"""
raylib GUI (backward-compatible facade).

The implementation is split across :mod:`hive_client.ui` and
:mod:`hive_client.app`; this module keeps the historical import path.
"""

from __future__ import annotations

from hive_client.domain.geometry import HEX_SIDES
from hive_client.ui.app import HiveGUI, run
from hive_client.ui.geometry import (
    HEX_ANGLE_STEP,
    HEX_SIZE,
    HEX_X_FACTOR,
    HEX_Y_FACTOR,
)
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
)
from hive_client.ui.renderer import HexRenderer
from hive_client.ui.theme import (
    OUTLINE_THICKNESS,
    OUTLINE_THICKNESS_SPECIAL,
    PIECE_COLORS,
    PIECE_LETTERS,
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
    "piece_letter",
    "run",
]
