"""
UI-agnostic game interaction controller.

The controller translates high-level user intents (``begin_palette_drag``,
``begin_board_drag``, ``drop_on_tile``, ``hover``) into legal moves submitted to
the native :class:`hive_native.Game`.  It contains no raylib, pixel, or windowing
code, so the complete interaction model is testable headlessly.
"""

from __future__ import annotations

from dataclasses import dataclass, field

import hive_native

from hive_client.app.view import DragView, GameView, StackView
from hive_client.domain.game import ORIGIN
from hive_client.domain.geometry import TilePointer
from hive_client.domain.moves import Move
from hive_client.domain.pieces import PIECE_KINDS, Piece, PieceKind, Player

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
    """Turn UI intents into validated native game mutations."""

    def __init__(self, game: hive_native.Game | None = None) -> None:
        """Wrap ``game`` (or start a fresh game)."""
        self.game = game or hive_native.Game()
        self.state = InteractionState()

    @property
    def current_player(self) -> Player:
        """Return the player whose turn it is."""
        return self.game.current_player

    # -- turn -------------------------------------------------------------

    def advance(self) -> None:
        """Advance the game by handling a forced pass, if one is needed."""
        if self.game.pass_if_stuck():
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
            if not self.game.can_place(kind):
                return frozenset()
            return frozenset(self._placement_targets())

        if tile is None:
            return frozenset()

        return self._move_targets(tile)

    def _placement_targets(self) -> tuple[TilePointer, ...]:
        return tuple(self.game.placement_targets())

    def _moves_from(self, tile: TilePointer) -> tuple[Move, ...]:
        return tuple(self.game.moves_from(tile))

    def _move_targets(self, tile: TilePointer) -> frozenset[TilePointer]:
        return frozenset(
            move.to_tile for move in self._moves_from(tile) if not move.is_placement
        )

    def _top(self, tile: TilePointer) -> Piece:
        return self.game.top(tile)

    def _clear_hover(self) -> None:
        self.state.hovered_tile = None
        self.state.hovered_kind = None
        self.state.hover_targets = frozenset()

    def _hover_hint(self, tile: TilePointer | None) -> str | None:
        if tile is None:
            return None

        if self.game.is_empty(tile):
            return None
        if self._top(tile).owner is not self.current_player:
            return None
        if not self.game.has_placed_queen(self.current_player):
            return "Place your Queen before moving pieces."
        return None

    # -- drag interaction -------------------------------------------------

    def begin_palette_drag(self, kind: PieceKind) -> bool:
        """Start dragging a fresh ``kind`` piece from the palette."""
        if not self.game.can_place(kind):
            return False

        self.state.drag = DragState(
            kind=kind,
            owner=self.current_player,
            from_tile=None,
            targets=frozenset(self._placement_targets()),
        )
        return True

    def begin_board_drag(self, tile: TilePointer) -> bool:
        """Start dragging an existing piece from ``tile``."""
        if not self.game.can_move_from(tile):
            return False

        top = self._top(tile)
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
            move = (
                Move.place(tile, drag.kind)
                if drag.from_tile is None
                else Move.move(drag.from_tile, tile, drag.kind)
            )
            applied = self.game.submit(move)

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
        stacks = tuple(
            StackView(tile=tile, pieces=tuple(pieces))
            for tile, pieces in self.game.stacks()
        )
        if self.game.is_empty_all():
            # The opening placement has no neighbouring hive yet, so expose the
            # centre tile as the only clickable target.
            clickable = {ORIGIN}
        else:
            clickable = self.game.occupied_tiles() | self.game.tiles_around_hive()

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
        remaining = self.game.remaining(self.current_player)

        return GameView(
            stacks=stacks,
            clickable_tiles=frozenset(clickable),
            current_player=self.current_player,
            result=self.game.result,
            remaining=tuple((kind, remaining[kind]) for kind in PIECE_KINDS),
            hovered_tile=hovered_tile,
            hover_targets=hover_targets,
            hint=self._hover_hint(hovered_tile),
            drag=drag_view,
        )
