"""
UI-agnostic game interaction controller.

The controller translates high-level user intents (``click_tile``,
``begin_palette_drag``, ``drop_on_tile``, …) into legal moves submitted to a
:class:`~hive_client.domain.game.GameSession`.  It contains no raylib, pixel,
or windowing code, so the complete interaction model is testable headlessly.
"""

from __future__ import annotations

from dataclasses import dataclass, field

from hive_client.app.view import DragView, GameView, StackView
from hive_client.domain.game import ORIGIN, GameSession
from hive_client.domain.geometry import TilePointer
from hive_client.domain.moves import Move, MovePiece, make_placement
from hive_client.domain.pieces import PIECE_KINDS, PieceKind, Player

__all__ = ["DragState", "GameController", "InteractionState"]


@dataclass(frozen=True)
class DragState:
    """A drag in progress, either from the palette or from the board."""

    kind: PieceKind
    owner: Player
    from_tile: TilePointer | None
    targets: frozenset[TilePointer]


@dataclass
class InteractionState:
    """Transient selection and drag state owned by the controller."""

    selected_kind: PieceKind = PieceKind.Queen
    selected_tile: TilePointer | None = None
    valid_moves: tuple[Move, ...] = field(default_factory=tuple)
    drag: DragState | None = None


class GameController:
    """Turn UI intents into validated game-session mutations."""

    def __init__(self, session: GameSession | None = None) -> None:
        """Wrap ``session`` (or start a fresh game)."""
        self.session = session or GameSession()
        self.state = InteractionState()

    # -- selection / turn -------------------------------------------------

    def select_kind(self, kind: PieceKind) -> None:
        """Choose which piece kind to place on the next tile click."""
        self.state.selected_kind = kind

    def advance(self) -> None:
        """Advance the game by handling a forced pass, if one is needed."""
        if self.session.pass_if_stuck():
            self._clear_selection()
            self.cancel_drag()

    # -- click interaction ------------------------------------------------

    def click_tile(self, tile: TilePointer) -> bool:
        """Handle a click on ``tile`` and return whether anything changed."""
        if self.session.is_over():
            return False

        # Opening: the first piece always goes at the centre.
        if self.session.board.is_empty_all():
            return self.session.submit(make_placement(tile, self.state.selected_kind))

        if self._execute_highlighted_move(tile):
            return True

        if self.session.board.is_empty(tile):
            return self._try_place_piece(tile)

        return self._select_or_deselect_tile(tile)

    def _execute_highlighted_move(self, tile: TilePointer) -> bool:
        if self.state.selected_tile == tile:
            self._clear_selection()
            return True

        for move in self.state.valid_moves:
            if isinstance(move, MovePiece) and move.to == tile:
                if self.session.submit(move):
                    self._clear_selection()
                return True
        return False

    def _try_place_piece(self, tile: TilePointer) -> bool:
        move = make_placement(tile, self.state.selected_kind)
        if not self.session.submit(move):
            return False
        self._clear_selection()
        return True

    def _select_or_deselect_tile(self, tile: TilePointer) -> bool:
        if self.state.selected_tile == tile:
            self._clear_selection()
            return True

        if not self.session.can_move_from(tile):
            return False

        self.state.selected_tile = tile
        self.state.valid_moves = self.session.moves_from(tile)
        return True

    # -- drag interaction -------------------------------------------------

    def begin_palette_drag(self, kind: PieceKind) -> bool:
        """Start dragging a fresh ``kind`` piece from the palette."""
        if not self.session.can_place(kind):
            return False

        self.state.drag = DragState(
            kind=kind,
            owner=self.session.current_player,
            from_tile=None,
            targets=frozenset(self.session.placement_targets()),
        )
        return True

    def begin_board_drag(self, tile: TilePointer) -> bool:
        """Start dragging an existing piece from ``tile``."""
        if not self.session.can_move_from(tile):
            return False

        top = self.session.board.get_top(tile)
        self.state.drag = DragState(
            kind=top.kind,
            owner=top.owner,
            from_tile=tile,
            targets=frozenset(
                move.to
                for move in self.session.moves_from(tile)
                if isinstance(move, MovePiece)
            ),
        )
        return True

    def drop_on_tile(self, tile: TilePointer) -> bool:
        """Complete the active drag on ``tile`` and return whether it moved."""
        drag = self.state.drag
        if drag is None:
            return False

        applied = False
        if tile in drag.targets:
            if drag.from_tile is None:
                applied = self.session.submit(make_placement(tile, drag.kind))
            else:
                applied = self.session.submit(
                    MovePiece(
                        from_=drag.from_tile,
                        to=tile,
                        piece_kind=drag.kind,
                    ),
                )

        self.cancel_drag()
        if applied:
            self._clear_selection()
        return applied

    def cancel_drag(self) -> None:
        """Abandon the active drag."""
        self.state.drag = None

    # -- view model -------------------------------------------------------

    def view(self) -> GameView:
        """Build an immutable snapshot for the renderer."""
        board = self.session.board
        stacks = tuple(
            StackView(tile=tile, pieces=pieces) for tile, pieces in board.stacks()
        )
        if board.is_empty_all():
            # The opening placement has no neighbouring hive yet, so expose the
            # centre tile as the only clickable target.
            clickable = {ORIGIN}
        else:
            clickable = board.occupied_tiles() | board.tiles_around_hive()

        drag = self.state.drag
        drag_view = (
            None
            if drag is None
            else DragView(
                kind=drag.kind,
                owner=drag.owner,
                from_tile=drag.from_tile,
                targets=drag.targets,
            )
        )

        move_targets = frozenset(
            move.to for move in self.state.valid_moves if isinstance(move, MovePiece)
        )
        remaining = board.remaining(self.session.current_player)

        return GameView(
            stacks=stacks,
            clickable_tiles=frozenset(clickable),
            current_player=self.session.current_player,
            result=self.session.result,
            remaining=tuple((kind, remaining[kind]) for kind in PIECE_KINDS),
            selected_kind=self.state.selected_kind,
            selected_tile=self.state.selected_tile,
            move_targets=move_targets,
            drag=drag_view,
        )

    def _clear_selection(self) -> None:
        self.state.selected_tile = None
        self.state.valid_moves = ()
