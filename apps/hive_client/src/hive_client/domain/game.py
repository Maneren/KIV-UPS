"""Game session: turn handling, legality checks, and win detection."""

from __future__ import annotations

from enum import Enum

from hive_client.domain.board import Board
from hive_client.domain.geometry import HEX_SIDES, TilePointer
from hive_client.domain.moves import Move, PlacePiece
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

    The session owns *all* rules that depend on whose turn it is.  Renderers and
    input handlers should never mutate ``board`` or ``current_player`` directly;
    they submit :class:`Move` objects through :meth:`submit` instead.
    """

    def __init__(self) -> None:
        """Start a new game with White to move."""
        self.board = Board()
        self.current_player = Player.White
        self.result = GameResult.ONGOING

    # -- queries --------------------------------------------------------

    def is_over(self) -> bool:
        """Return True once the game has a result."""
        return self.result is not GameResult.ONGOING

    @property
    def winner(self) -> Player | None:
        """Return the winning player, or None while the game is ongoing/drawn."""
        if self.result is GameResult.WHITE_WINS:
            return Player.White
        if self.result is GameResult.BLACK_WINS:
            return Player.Black
        return None

    def legal_moves(self) -> list[Move]:
        """
        Return all legal moves for the current player.

        On an empty board the only legal action is the opening placement, which
        is exposed through :meth:`placement_targets` instead.  This mirrors the
        board engine, which needs an existing hive to generate moves.
        """
        return list(
            self.board.moves_for_player(
                self.current_player,
                self.board.remaining(self.current_player),
            )
        )

    def has_legal_move(self) -> bool:
        """Return True if the current player has any legal move or placement."""
        moves = self.board.moves_for_player(
            self.current_player,
            self.board.remaining(self.current_player),
        )
        return next(moves, None) is not None

    def can_place(self, kind: PieceKind) -> bool:
        """Return True if the current player may still place ``kind``."""
        return not self.is_over() and self.board.can_player_place(
            self.current_player, kind
        )

    def can_place_at(self, ptr: TilePointer) -> bool:
        """Return True if the current player may place a piece at ``ptr``."""
        if self.is_over():
            return False
        if self.board.is_empty_all():
            return ptr == ORIGIN
        return self.board.can_player_place_at(self.current_player, ptr)

    def placement_targets(self) -> tuple[TilePointer, ...]:
        """Return every tile where the current player may place a piece."""
        if self.is_over():
            return ()
        if self.board.is_empty_all():
            return (
                (ORIGIN,)
                if any(self.board.remaining(self.current_player).values())
                else ()
            )
        return tuple(self.board.valid_placements(self.current_player))

    def can_move_from(self, ptr: TilePointer) -> bool:
        """Return True if the current player may move the piece at ``ptr``."""
        return (
            not self.is_over()
            and not self.board.is_empty(ptr)
            and self.board.get_top(ptr).owner is self.current_player
            and self.board.can_player_move(self.current_player, ptr)
        )

    def moves_from(self, ptr: TilePointer) -> tuple[Move, ...]:
        """Return every legal move for the piece at ``ptr``, if it is ours."""
        if not self.can_move_from(ptr):
            return ()
        top = self.board.get_top(ptr)
        return tuple(self.board.moves_for_piece(ptr, top))

    def is_legal(self, move: Move) -> bool:
        """Check ``move`` against the current turn, supply, and movement rules."""
        if self.is_over():
            return False

        if isinstance(move, PlacePiece):
            return self.can_place(move.piece_kind) and self.can_place_at(move.pos)

        return any(candidate == move for candidate in self.moves_from(move.from_))

    # -- mutation -------------------------------------------------------

    def submit(self, move: Move) -> bool:
        """Apply ``move`` if legal and return whether it was accepted."""
        if not self.is_legal(move):
            return False
        self.apply(move)
        return True

    def apply(self, move: Move) -> None:
        """
        Apply ``move`` on behalf of the current player, then switch turns.

        This is the low-level transition used after validation; prefer
        :meth:`submit` in UI/input code.
        """
        self.board.apply_move(move, self.current_player)
        self.current_player = self.current_player.other()
        self.check_win()

    def pass_if_stuck(self) -> bool:
        """
        Pass the turn if the current player has no legal action.

        Returns True if the turn was passed.  The opening placement is handled
        separately, so an empty board never triggers a pass.
        """
        if self.is_over() or self.board.is_empty_all() or self.has_legal_move():
            return False
        self.current_player = self.current_player.other()
        return True

    # -- outcome --------------------------------------------------------

    def check_win(self) -> None:
        """Set ``result`` if either queen is surrounded."""
        for player in (Player.White, Player.Black):
            queen_pos = self.queen_position(player)
            if queen_pos is None:
                continue
            if self.board.surrounding_count(queen_pos) == HEX_SIDES:
                self.result = (
                    GameResult.BLACK_WINS
                    if player is Player.White
                    else GameResult.WHITE_WINS
                )
                return

    def queen_position(self, player: Player) -> TilePointer | None:
        """Return the tile of ``player``'s queen, or None if not placed."""
        for ptr, piece in self.board.pieces():
            if piece.owner is player and piece.kind is PieceKind.Queen:
                return ptr
        return None
