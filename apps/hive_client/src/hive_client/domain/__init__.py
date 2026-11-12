"""
Hive game domain: a Python adapter over the native engine.

Value objects, board views, and game sessions live here; the rules themselves
are implemented in ``src/hive`` and exposed by :mod:`hive_native`.  Nothing in
this package imports a GUI, networking, or the wire protocol, so it can be
exercised headlessly.
"""

from hive_client.domain.board import BoardView
from hive_client.domain.errors import EmptyTileError, HiveError, IllegalMoveError
from hive_client.domain.game import ORIGIN, GameResult, GameSession
from hive_client.domain.geometry import (
    DIRECTIONS,
    HEX_SIDES,
    Direction,
    TilePointer,
    neighboring_cells,
    rotate_left,
    rotate_right,
)
from hive_client.domain.moves import (
    Move,
    MovePiece,
    PlacePiece,
    make_placement,
)
from hive_client.domain.pieces import (
    DEFAULT_PIECES_PER_PLAYER,
    PIECE_KINDS,
    Piece,
    PieceKind,
    Player,
    make_initial_supply,
)

__all__ = [
    "DEFAULT_PIECES_PER_PLAYER",
    "DIRECTIONS",
    "HEX_SIDES",
    "ORIGIN",
    "PIECE_KINDS",
    "BoardView",
    "Direction",
    "EmptyTileError",
    "GameResult",
    "GameSession",
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
    "make_placement",
    "neighboring_cells",
    "rotate_left",
    "rotate_right",
]
