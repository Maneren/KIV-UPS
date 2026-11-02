"""
Wire-protocol helpers for the Hive game.

Maps between `PieceKind` enum values (used internally by the game engine)
and the single-letter piece codes used on the wire protocol
(see src/hive/PROTOCOL.md).
"""

from __future__ import annotations

from hive_client.game import PieceKind

__all__ = [
    "PIECE_KIND_TO_LETTER",
    "PIECE_LETTER_TO_KIND",
    "piece_kind_from_letter",
    "piece_letter_from_kind",
]

PIECE_LETTER_TO_KIND: dict[str, PieceKind] = {
    "Q": PieceKind.Queen,
    "S": PieceKind.Spider,
    "B": PieceKind.Beetle,
    "G": PieceKind.Grasshopper,
    "A": PieceKind.Ant,
}

PIECE_KIND_TO_LETTER: dict[PieceKind, str] = {
    kind: letter for letter, kind in PIECE_LETTER_TO_KIND.items()
}


def piece_kind_from_letter(letter: str) -> PieceKind:
    """Convert a wire-protocol piece letter to a PieceKind."""
    try:
        return PIECE_LETTER_TO_KIND[letter.upper()]
    except KeyError as exc:
        raise ValueError(f"unknown piece letter: {letter!r}") from exc


def piece_letter_from_kind(kind: PieceKind) -> str:
    """Convert a PieceKind to its wire-protocol letter."""
    try:
        return PIECE_KIND_TO_LETTER[kind]
    except KeyError as exc:
        raise ValueError(f"unknown piece kind: {kind!r}") from exc
