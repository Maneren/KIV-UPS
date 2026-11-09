"""Colors, typography, and key bindings for the raylib UI."""

from __future__ import annotations

from pyray import (
    BLUE,
    DARKGREEN,
    GOLD,
    GRAY,
    PURPLE,
    RED,
    Color,
    color_tint,
)

from hive_client.domain.pieces import Piece, Player

__all__ = [
    "OUTLINE_THICKNESS",
    "OUTLINE_THICKNESS_SPECIAL",
    "PIECE_COLORS",
    "PIECE_LETTERS",
    "TEXT_FONT_SIZE",
    "TUTORIAL_FONT_SIZE",
    "TUTORIAL_LINES",
    "TUTORIAL_TOGGLE_KEY",
    "piece_color",
    "piece_letter",
]

OUTLINE_THICKNESS = 2.0
OUTLINE_THICKNESS_SPECIAL = 3.0
TEXT_FONT_SIZE = 24
TUTORIAL_FONT_SIZE = 16
TUTORIAL_TOGGLE_KEY = 72  # 'H'

PIECE_COLORS: dict[str, Color] = {
    "Queen": GOLD,
    "Spider": RED,
    "Ant": BLUE,
    "Beetle": PURPLE,
    "Grasshopper": DARKGREEN,
}

PIECE_LETTERS: dict[str, str] = {
    "Queen": "Q",
    "Spider": "S",
    "Ant": "A",
    "Beetle": "B",
    "Grasshopper": "G",
}

TUTORIAL_LINES: list[str] = [
    "Hive - how to play",
    "",
    "* Drag a piece from the left",
    "  palette onto the board.",
    "  The first piece must go at",
    "  the board center (0, 0).",
    "",
    "* Drag your own piece to a",
    "  green ring to move it.",
    "  Hover a piece to preview",
    "  its legal moves.",
    "",
    "* Place your Queen before",
    "  moving any piece.",
    "",
    "* Queen moves 1 step.",
    "* Spider moves exactly 3 steps.",
    "* Beetle moves onto any",
    "  neighbouring occupied cell.",
    "* Grasshopper jumps over a line",
    "  of pieces.",
    "* Ant moves any number of steps",
    "  around the hive.",
    "",
    "Goal: surround the enemy queen",
    "with pieces (any colour).",
]


def piece_letter(piece: Piece) -> str:
    """Return the display letter, lowercase for Black."""
    letter = PIECE_LETTERS[piece.kind.name]
    if piece.owner is Player.Black:
        return letter.lower()
    return letter


def piece_color(piece: Piece) -> Color:
    """Return the piece color: kind colour, darkened for Black."""
    base = PIECE_COLORS[piece.kind.name]
    if piece.owner is Player.Black:
        return color_tint(base, GRAY)
    return base
