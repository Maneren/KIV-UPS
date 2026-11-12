"""Game session backed by the native Hive engine."""

from __future__ import annotations

from enum import Enum
from typing import cast

import hive_native

from hive_client.domain._convert import move_from_record, move_to_record
from hive_client.domain.board import BoardView
from hive_client.domain.geometry import TilePointer
from hive_client.domain.moves import Move
from hive_client.domain.pieces import PieceKind, Player

__all__ = ["ORIGIN", "GameResult", "GameSession"]

#: The first piece of the game is always placed at the board centre.
ORIGIN = TilePointer(0, 0)


class GameResult(Enum):
    """Outcome of a Hive game."""

    ONGOING = 0
    WHITE_WINS = 1
    BLACK_WINS = 2
    DRAW = 3


class GameSession:
    """
    A Hive game: board, current player, opening rule, and outcome.

    Every rule and query is delegated to :class:`hive_native.Game`.  Renderers
    and input handlers should never mutate state directly; they submit
    :class:`Move` objects through :meth:`submit` instead.
    """

    def __init__(self) -> None:
        """Start a new game with White to move."""
        self._game = hive_native.Game()
        self.board = BoardView(self._game)

    # -- queries --------------------------------------------------------

    @property
    def current_player(self) -> Player:
        """Return the player whose turn it is."""
        return Player(self._game.current_player)

    @property
    def result(self) -> GameResult:
        """Return the current outcome of the game."""
        return GameResult(self._game.result)

    @property
    def winner(self) -> Player | None:
        """Return the winning player, or None while the game is ongoing/drawn."""
        winner = self._game.winner
        return None if winner is None else Player(winner)

    def is_over(self) -> bool:
        """Return True once the game has a result."""
        return self._game.is_over()

    def legal_moves(self) -> list[Move]:
        """Return all legal moves for the current player."""
        return [move_from_record(record) for record in self._game.legal_moves()]

    def has_legal_move(self) -> bool:
        """Return True if the current player has any legal move or placement."""
        return self._game.has_legal_move()

    def can_place(self, kind: PieceKind) -> bool:
        """Return True if the current player may still place ``kind``."""
        return self._game.can_place(kind.value)

    def can_place_at(self, ptr: TilePointer) -> bool:
        """Return True if the current player may place a piece at ``ptr``."""
        return self._game.can_place_at(ptr.p, ptr.q)

    def placement_targets(self) -> tuple[TilePointer, ...]:
        """Return every tile where the current player may place a piece."""
        return tuple(TilePointer(p, q) for p, q in self._game.placement_targets())

    def can_move_from(self, ptr: TilePointer) -> bool:
        """Return True if the current player may move the piece at ``ptr``."""
        return self._game.can_move_from(ptr.p, ptr.q)

    def moves_from(self, ptr: TilePointer) -> tuple[Move, ...]:
        """Return every legal move for the piece at ``ptr``, if it is ours."""
        return tuple(
            move_from_record(record) for record in self._game.moves_from(ptr.p, ptr.q)
        )

    def is_legal(self, move: Move) -> bool:
        """Check ``move`` against the current turn, supply, and movement rules."""
        return self._game.is_legal(move_to_record(move))

    # -- mutation -------------------------------------------------------

    def submit(self, move: Move) -> bool:
        """Apply ``move`` if legal and return whether it was accepted."""
        return self._game.submit(move_to_record(move))

    def apply(self, move: Move) -> None:
        """
        Apply ``move`` on behalf of the current player, then switch turns.

        This is the low-level transition used after validation; prefer
        :meth:`submit` in UI/input code.
        """
        self._game.apply(move_to_record(move))

    def pass_if_stuck(self) -> bool:
        """Pass the turn if the current player has no legal action."""
        return self._game.pass_if_stuck()

    # -- outcome --------------------------------------------------------

    def queen_position(self, player: Player) -> TilePointer | None:
        """Return the tile of ``player``'s queen, or None if not placed."""
        position = self._game.queen_position(player.value)
        if position is None:
            return None
        p, q = cast("tuple[int, int]", position)
        return TilePointer(p, q)
