"""
UI-agnostic game interaction controller.

The controller translates high-level user intents (``begin_palette_drag``,
``begin_board_drag``, ``drop_on_tile``, ``hover``) into legal moves submitted to
a :class:`~hive_client.domain.game.GameSession`.  It contains no raylib, pixel,
or windowing code, so the complete interaction model is testable headlessly.
"""

from __future__ import annotations

from dataclasses import dataclass, field

from hive_client.app.view import DragView, GameView, StackView
from hive_client.domain.game import ORIGIN, GameSession
from hive_client.domain.geometry import TilePointer
from hive_client.domain.moves import MovePiece, make_placement
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
    """Transient hover and drag state owned by the controller."""

    hovered_tile: TilePointer | None = None
    hovered_kind: PieceKind | None = None
    hover_targets: frozenset[TilePointer] = field(default_factory=frozenset)
    drag: DragState | None = None


class GameController:
    """Turn UI intents into validated game-session mutations."""

    def __init__(self, session: GameSession | None = None) -> None:
        """Wrap ``session`` (or start a fresh game)."""
        self.session = session or GameSession()
        self.state = InteractionState()

    # -- turn -------------------------------------------------------------

    def advance(self) -> None:
        """Advance the game by handling a forced pass, if one is needed."""
        if self.session.pass_if_stuck():
            self._clear_hover()
            self.cancel_drag()

    # -- hover interaction ------------------------------------------------

    def hover(
        self,
        *,
        tile: TilePointer | None = None,
        kind: PieceKind | None = None,
    ) -> None:
        """Preview legal targets for the piece under the cursor."""
        if (tile, kind) == (self.state.hovered_tile, self.state.hovered_kind):
            return

        self.state.hovered_tile = tile
        self.state.hovered_kind = kind
        self.state.hover_targets = self._targets_for(tile, kind)

    def _targets_for(
        self,
        tile: TilePointer | None,
        kind: PieceKind | None,
    ) -> frozenset[TilePointer]:
        if kind is not None:
            if not self.session.can_place(kind):
                return frozenset()
            return frozenset(self.session.placement_targets())

        if tile is None:
            return frozenset()

        return self._move_targets(tile)

    def _move_targets(self, tile: TilePointer) -> frozenset[TilePointer]:
        return frozenset(
            move.to
            for move in self.session.moves_from(tile)
            if isinstance(move, MovePiece)
        )

    def _clear_hover(self) -> None:
        self.state.hovered_tile = None
        self.state.hovered_kind = None
        self.state.hover_targets = frozenset()

    def _hover_hint(self, tile: TilePointer | None) -> str | None:
        if tile is None:
            return None

        board = self.session.board
        if board.is_empty(tile):
            return None
        if board.get_top(tile).owner is not self.session.current_player:
            return None
        if not board.has_placed_queen(self.session.current_player):
            return "Place your Queen before moving pieces."
        return None

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
            targets=self._move_targets(tile),
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
            self._clear_hover()
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
        # A drag already shows its own targets; suppress the hover preview.
        hovered_tile = None if drag is not None else self.state.hovered_tile
        hover_targets = frozenset() if drag is not None else self.state.hover_targets
        remaining = board.remaining(self.session.current_player)

        return GameView(
            stacks=stacks,
            clickable_tiles=frozenset(clickable),
            current_player=self.session.current_player,
            result=self.session.result,
            remaining=tuple((kind, remaining[kind]) for kind in PIECE_KINDS),
            hovered_tile=hovered_tile,
            hover_targets=hover_targets,
            hint=self._hover_hint(hovered_tile),
            drag=drag_view,
        )
