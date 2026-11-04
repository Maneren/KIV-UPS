"""Colors, typography, and key bindings for the raylib UI."""

from __future__ import annotations

from pyray import (
    BLUE,
    DARKGREEN,
    GOLD,
    PURPLE,
    RED,
    Color,
)

from hive_client.domain.pieces import Piece, PieceKind, Player

__all__ = [
    "OUTLINE_THICKNESS",
    "OUTLINE_THICKNESS_SPECIAL",
    "PIECE_COLORS",
    "PIECE_LETTERS",
    "PIECE_SELECT_KEYS",
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

PIECE_SELECT_KEYS: dict[int, PieceKind] = {
    81: PieceKind.Queen,  # 'Q'
    83: PieceKind.Spider,  # 'S'
    66: PieceKind.Beetle,  # 'B'
    71: PieceKind.Grasshopper,  # 'G'
    65: PieceKind.Ant,  # 'A'
}

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
    "Hive — how to play",
    "",
    "• Click an empty hex to place",
    "  your selected piece.",
    "  The first piece must go at",
    "  the board center (0, 0).",
    "",
    "• Click your own piece to",
    "  select it and show its valid",
    "  moves (green rings).",
    "",
    "• Click a green ring to move",
    "  the piece there.",
    "",
    "• Queen moves 1 step.",
    "• Spider moves exactly 3 steps.",
    "• Beetle moves onto any",
    "  neighbouring occupied cell.",
    "• Grasshopper jumps over a line",
    "  of pieces.",
    "• Ant moves any number of steps",
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
    if hasattr(base, "r"):
        r, g, b, a = base.r, base.g, base.b, base.a
    else:  # pragma: no cover - older raylib bindings expose tuples
        r, g, b, a = base
    if piece.owner is Player.Black:
        return Color(
            max(0, int(r * 0.5)),
            max(0, int(g * 0.5)),
            max(0, int(b * 0.5)),
            a,
        )
    return Color(r, g, b, a)
