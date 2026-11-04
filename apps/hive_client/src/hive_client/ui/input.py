"""
Translate raylib mouse events into controller intents.

Hit-testing (pixels -> tiles / palette entries) lives here, in the UI layer.
The controller only ever sees domain coordinates and piece kinds.
"""

from __future__ import annotations

from dataclasses import dataclass

from hive_client.app.controller import DragState, GameController
from hive_client.domain.geometry import TilePointer
from hive_client.domain.pieces import PieceKind
from hive_client.ui.geometry import Point, pixel_to_tile
from hive_client.ui.layout import Rect, palette_tiles

__all__ = ["InputRouter", "MouseEvent"]


@dataclass
class MouseEvent:
    """Mouse state for a single frame."""

    pressed: bool
    released: bool
    position: Point


class InputRouter:
    """Route mouse input to a :class:`GameController`."""

    def __init__(self, controller: GameController) -> None:
        """Route events to ``controller``."""
        self.controller = controller

    def handle(
        self,
        event: MouseEvent,
        center: Point,
        palette: tuple[tuple[PieceKind, Rect], ...] | None = None,
    ) -> None:
        """Process one frame of mouse input."""
        palette = palette if palette is not None else palette_tiles()

        if event.pressed:
            self._on_press(event.position, center, palette)
        if event.released:
            self._on_release(event.position, center, palette)

    def _on_press(
        self,
        point: Point,
        center: Point,
        palette: tuple[tuple[PieceKind, Rect], ...],
    ) -> None:
        kind = _palette_kind_at(point, palette)
        if kind is not None:
            # Selecting on press means a simple click also picks the kind,
            # while holding and dragging starts a placement drag.
            self.controller.select_kind(kind)
            self.controller.begin_palette_drag(kind)
            return

        tile = self._tile_at(point, center)
        if tile is not None:
            self.controller.begin_board_drag(tile)

    def _on_release(
        self,
        point: Point,
        center: Point,
        palette: tuple[tuple[PieceKind, Rect], ...],
    ) -> None:
        drag = self.controller.state.drag
        if drag is not None:
            self._release_drag(point, center, palette, drag)
            return

        tile = self._tile_at(point, center)
        if tile is not None:
            self.controller.click_tile(tile)

    def _release_drag(
        self,
        point: Point,
        center: Point,
        palette: tuple[tuple[PieceKind, Rect], ...],
        drag: DragState,
    ) -> None:
        # A click on the palette without dragging just selects the kind.
        if drag.from_tile is None and _palette_kind_at(point, palette) is drag.kind:
            self.controller.cancel_drag()
            return

        tile = self._tile_at(point, center)

        # Clicking a board piece without moving it toggles its selection.
        if drag.from_tile is not None and tile == drag.from_tile:
            self.controller.cancel_drag()
            self.controller.click_tile(drag.from_tile)
            return

        if tile is not None and self.controller.drop_on_tile(tile):
            return

        self.controller.cancel_drag()

    def _tile_at(self, point: Point, center: Point) -> TilePointer | None:
        tile = pixel_to_tile(point, center)
        view = self.controller.view()
        return tile if tile in view.clickable_tiles else None


def _palette_kind_at(
    point: Point,
    palette: tuple[tuple[PieceKind, Rect], ...],
) -> PieceKind | None:
    for kind, rect in palette:
        if rect.contains(point):
            return kind
    return None
