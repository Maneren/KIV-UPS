"""
Pure Hive game domain: rules, board state, and game sessions.

Nothing in this package imports a GUI, networking, or the wire protocol, so it
can be exercised headlessly.
"""

from hive_client.domain.board import MAX_STACK_IGNORE, Board
from hive_client.domain.errors import EmptyTileError, HiveError, IllegalMoveError
from hive_client.domain.game import ORIGIN, GameResult, GameSession, GameState
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
    make_move,
    make_placement,
)
from hive_client.domain.pieces import (
    DEFAULT_PIECES_PER_PLAYER,
    NUMBER_OF_PIECES,
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
    "MAX_STACK_IGNORE",
    "NUMBER_OF_PIECES",
    "ORIGIN",
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
