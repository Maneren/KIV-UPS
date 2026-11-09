"""
Translate raylib mouse events into controller intents.

Hit-testing (pixels -> tiles / palette entries) lives here, in the UI layer.
The controller only ever sees domain coordinates and piece kinds.
"""

from __future__ import annotations

from dataclasses import dataclass

from pyray import check_collision_point_rec

from hive_client.app.controller import GameController
from hive_client.domain.geometry import TilePointer
from hive_client.domain.pieces import PieceKind
from hive_client.ui.geometry import Point, pixel_to_tile
from hive_client.ui.layout import PALETTE_BOUNDS, PALETTE_PIECES

__all__ = ["MouseEvent", "MouseRouter"]


@dataclass
class MouseEvent:
    """Mouse state for a single frame."""

    pressed: bool
    released: bool
    position: Point


class MouseRouter:
    """Route mouse input to a :class:`GameController`."""

    def __init__(self, controller: GameController) -> None:
        """Route events to ``controller``."""
        self.controller = controller

    def handle(
        self,
        event: MouseEvent,
        center: Point,
    ) -> None:
        """Process one frame of mouse input and update the hover preview."""
        if event.pressed:
            self._on_press(event.position, center)
        if event.released:
            self._on_release(event.position, center)

        if self.controller.state.drag is None:
            self._update_hover(event.position, center)

    def _on_press(
        self,
        point: Point,
        center: Point,
    ) -> None:
        if (kind := _palette_kind_at(point)) is not None:
            self.controller.begin_palette_drag(kind)
            return

        if (tile := self._tile_at(point, center)) is not None:
            self.controller.begin_board_drag(tile)

    def _on_release(
        self,
        point: Point,
        center: Point,
    ) -> None:
        if self.controller.state.drag is None:
            return

        tile = self._tile_at(point, center)
        if tile is not None and self.controller.drop_on_tile(tile):
            return

        self.controller.cancel_drag()

    def _update_hover(self, point: Point, center: Point) -> None:
        if (kind := _palette_kind_at(point)) is not None:
            self.controller.hover(kind=kind)
            return

        self.controller.hover(tile=self._tile_at(point, center))

    def _tile_at(self, point: Point, center: Point) -> TilePointer | None:
        tile = pixel_to_tile(point, center)
        view = self.controller.view()
        return tile if tile in view.clickable_tiles else None


def _palette_kind_at(point: Point) -> PieceKind | None:
    if not check_collision_point_rec(point, PALETTE_BOUNDS):
        return None

    kinds = (
        kind for kind, rect in PALETTE_PIECES if check_collision_point_rec(point, rect)
    )

    return next(kinds, None)
