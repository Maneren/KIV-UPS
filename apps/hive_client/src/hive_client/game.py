"""
Hive game engine (backward-compatible facade).

The implementation lives in :mod:`hive_client.domain`.  New code should import
from ``hive_client.domain`` directly; this module keeps the historical
``hive_client.game`` import path working.
"""

from __future__ import annotations

from hive_client.domain import (
    DEFAULT_PIECES_PER_PLAYER,
    DIRECTIONS,
    HEX_SIDES,
    MAX_STACK_IGNORE,
    NUMBER_OF_PIECES,
    PIECE_KINDS,
    Board,
    Direction,
    EmptyTileError,
    GameResult,
    GameSession,
    GameState,
    HiveError,
    IllegalMoveError,
    Move,
    MovePiece,
    Piece,
    PieceKind,
    PlacePiece,
    Player,
    TilePointer,
    make_initial_supply,
    make_move,
    make_placement,
    neighboring_cells,
    rotate_left,
    rotate_right,
)

__all__ = [
    "DEFAULT_PIECES_PER_PLAYER",
    "DIRECTIONS",
    "HEX_SIDES",
    "MAX_STACK_IGNORE",
    "NUMBER_OF_PIECES",
    "PIECE_KINDS",
    "Board",
    "Direction",
    "EmptyTileError",
    "GameResult",
    "GameSession",
    "GameState",
    "HiveError",
    "IllegalMoveError",
    "Move",
    "MovePiece",
    "Piece",
    "PieceKind",
    "PlacePiece",
    "Player",
    "TilePointer",
    "make_initial_supply",
    "make_move",
    "make_placement",
    "neighboring_cells",
    "rotate_left",
    "rotate_right",
]
