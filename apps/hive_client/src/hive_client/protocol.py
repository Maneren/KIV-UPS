"""
Wire-protocol helpers (backward-compatible facade).

The implementation lives in :mod:`hive_client.wire.codec`.
"""

from __future__ import annotations

from hive_client.domain.pieces import PieceKind
from hive_client.wire.codec import (
    PIECE_KIND_TO_LETTER,
    PIECE_LETTER_TO_KIND,
    move_from_wire,
    move_to_wire,
    parse_command,
    piece_kind_from_letter,
    piece_letter_from_kind,
    place_from_wire,
    place_to_wire,
    serialize,
)

__all__ = [
    "PIECE_KIND_TO_LETTER",
    "PIECE_LETTER_TO_KIND",
    "PieceKind",
    "move_from_wire",
    "move_to_wire",
    "parse_command",
    "piece_kind_from_letter",
    "piece_letter_from_kind",
    "place_from_wire",
    "place_to_wire",
    "serialize",
]
