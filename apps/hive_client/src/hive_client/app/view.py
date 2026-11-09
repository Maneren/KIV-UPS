"""
Immutable, render-ready snapshots of a game session.

The UI layer consumes :class:`GameView` and never touches ``Board`` or
``GameSession`` directly.  This keeps rule queries out of the renderer and
makes the view model easy to test.
"""

from __future__ import annotations

from dataclasses import dataclass

from hive_client.domain.game import GameResult
from hive_client.domain.geometry import TilePointer
from hive_client.domain.pieces import Piece, PieceKind, Player

__all__ = ["DragView", "GameView", "StackView"]


@dataclass(frozen=True)
class StackView:
    """A tile and the pieces stacked on it, bottom first."""

    tile: TilePointer
    pieces: tuple[Piece, ...]


@dataclass(frozen=True)
class DragView:
    """The piece currently being dragged and the tiles accepting it."""

    kind: PieceKind
    owner: Player
    from_tile: TilePointer | None
    targets: frozenset[TilePointer]


@dataclass(frozen=True)
class GameView:
    """Everything the renderer needs for one frame."""

    stacks: tuple[StackView, ...]
    clickable_tiles: frozenset[TilePointer]
    current_player: Player
    result: GameResult
    remaining: tuple[tuple[PieceKind, int], ...]
    hovered_tile: TilePointer | None
    hover_targets: frozenset[TilePointer]
    hint: str | None
    drag: DragView | None

    @property
    def remaining_map(self) -> dict[PieceKind, int]:
        """Return remaining pieces as a mapping for convenient lookups."""
        return dict(self.remaining)
