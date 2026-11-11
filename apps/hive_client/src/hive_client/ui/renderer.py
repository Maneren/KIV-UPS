"""raylib rendering for :class:`~hive_client.app.view.GameView`."""

from __future__ import annotations

import math
from dataclasses import dataclass

from pyray import (
    BLACK,
    BLUE,
    DARKGRAY,
    GREEN,
    RAYWHITE,
    RED,
    WHITE,
    Color,
    Vector2,
    begin_drawing,
    clear_background,
    draw_line_ex,
    draw_poly,
    draw_poly_lines_ex,
    draw_rectangle,
    draw_text,
    end_drawing,
)

from hive_client.app.view import GameView
from hive_client.domain.geometry import HEX_SIDES, TilePointer
from hive_client.domain.pieces import Piece, Player
from hive_client.ui.geometry import (
    HEX_SIZE,
    Point,
    hex_to_pixel,
    snap_to_pixel,
)
from hive_client.ui.layout import (
    PALETTE_PIECES,
    PALETTE_TILE_SIZE,
    TUTORIAL_PANEL_WIDTH,
    TUTORIAL_X_OFFSET,
    TUTORIAL_Y_OFFSET,
    rect_center,
)
from hive_client.ui.text import (
    draw_text_centered,
    draw_text_centered_x,
    draw_text_centered_y,
    wrap_text,
)
from hive_client.ui.theme import (
    OUTLINE_THICKNESS,
    OUTLINE_THICKNESS_SPECIAL,
    PIECE_LETTERS,
    TEXT_FONT_SIZE,
    TUTORIAL_FONT_SIZE,
    TUTORIAL_LINES,
    piece_color,
    piece_letter,
)

__all__ = ["HexRenderer", "RenderContext"]


@dataclass(frozen=True)
class RenderContext:
    """Everything a frame needs; keeps the renderer free of global state."""

    view: GameView
    width: float
    height: float
    center: Point
    mouse: Point
    tutorial_visible: bool


def _vec(point: Point) -> Vector2:
    return Vector2(point[0], point[1])


class HexRenderer:
    """Pure rendering: board, pieces, highlights, tutorial, labels."""

    def __init__(self) -> None:
        """Create a renderer with the default panel background."""
        self.background = Color(240, 240, 240, 255)

    def draw(self, context: RenderContext) -> None:
        """Draw one complete frame."""
        view = context.view
        begin_drawing()
        clear_background(RAYWHITE)

        if context.tutorial_visible:
            self._draw_tutorial(context.width, context.height)
        self._draw_turn_label(view.current_player, context.center[0])
        self._draw_hint(view.hint, context.center[0])
        self._draw_tiles(view, context.center)
        self._draw_palette(view)
        self._draw_drag(view, context.center, context.mouse)

        end_drawing()

    # -- panels -----------------------------------------------------------

    def _draw_tutorial(self, screen_width: float, screen_height: float) -> None:
        panel_width = TUTORIAL_PANEL_WIDTH
        panel_x = screen_width - panel_width
        panel_height = screen_height
        draw_rectangle(
            int(panel_x),
            0,
            int(panel_width),
            int(panel_height),
            self.background,
        )
        draw_line_ex(
            Vector2(panel_x, 0),
            Vector2(panel_x, panel_height),
            2.0,
            DARKGRAY,
        )

        text_width = panel_width - 2 * TUTORIAL_X_OFFSET
        text_x = int(panel_x + TUTORIAL_X_OFFSET)
        line_height = TUTORIAL_FONT_SIZE + 4

        y = TUTORIAL_Y_OFFSET
        for line in TUTORIAL_LINES:
            for wrapped in wrap_text(line, text_width, TUTORIAL_FONT_SIZE):
                if wrapped:
                    draw_text(wrapped, text_x, int(y), TUTORIAL_FONT_SIZE, DARKGRAY)

                y += line_height

    def _draw_turn_label(self, current_player: Player, center_x: float) -> None:
        turn_text = f"{current_player.name}'s turn"
        draw_text_centered_x(turn_text, center_x, 10, TEXT_FONT_SIZE, DARKGRAY)

    def _draw_hint(self, hint: str | None, center_x: float) -> None:
        if hint is not None:
            draw_text_centered_x(hint, center_x, 40, TEXT_FONT_SIZE, DARKGRAY)

    # -- board ------------------------------------------------------------

    def _draw_tiles(self, view: GameView, center: Point) -> None:
        drag_from = view.drag.from_tile if view.drag is not None else None

        for stack in view.stacks:
            pos = hex_to_pixel(stack.tile, center)
            # Skip the top piece at the drag origin (it follows the mouse).
            pieces = stack.pieces if stack.tile != drag_from else stack.pieces[:-1]
            for i, stack_piece in enumerate(pieces):
                size = HEX_SIZE * math.pow(0.85, i)
                _draw_piece_tile(
                    pos,
                    stack_piece,
                    size=size,
                    font_size=TEXT_FONT_SIZE,
                )

        for ptr in view.hover_targets:
            _draw_tile_outline(ptr, center, GREEN)

        if drag_from is not None:
            _draw_tile_outline(drag_from, center, BLUE)

        if view.hint is not None and view.hovered_tile is not None:
            _draw_tile_outline(view.hovered_tile, center, RED)

    def _draw_palette(self, view: GameView) -> None:
        """Draw draggable piece tiles with remaining counts."""
        remaining = view.remaining_map
        for kind, rect in PALETTE_PIECES:
            center_x, center_y = rect_center(rect)
            is_dragging = (
                view.drag is not None
                and view.drag.from_tile is None
                and view.drag.kind is kind
            )
            _draw_piece_tile(
                (center_x, center_y),
                Piece(kind=kind, owner=view.current_player),
                size=PALETTE_TILE_SIZE / 2,
                font_size=int(PALETTE_TILE_SIZE * 0.5),
                letter=PIECE_LETTERS[kind.name],
            )

            draw_text_centered_y(
                f": {remaining.get(kind, 0)}",
                rect.x + PALETTE_TILE_SIZE + 6,
                center_y,
                TEXT_FONT_SIZE,
                DARKGRAY,
            )

            if is_dragging:
                _draw_hex_outline(
                    (center_x, center_y),
                    GREEN,
                    OUTLINE_THICKNESS_SPECIAL,
                    PALETTE_TILE_SIZE / 2,
                )

    # -- drag overlay -----------------------------------------------------

    def _draw_drag(self, view: GameView, center: Point, mouse: Point) -> None:
        if view.drag is None:
            return

        for ptr in view.drag.targets:
            _draw_tile_outline(ptr, center, GREEN)

        snapped = snap_to_pixel(mouse, center)
        _draw_piece_tile(
            snapped,
            Piece(kind=view.drag.kind, owner=view.drag.owner),
            size=HEX_SIZE * 0.9,
            font_size=int(TEXT_FONT_SIZE * 0.9),
            letter=PIECE_LETTERS[view.drag.kind.name],
            text_color=WHITE,
        )


def _draw_piece_tile(  # noqa: PLR0913 - visual style knobs shared by callers
    position: Point,
    piece: Piece,
    *,
    size: float,
    font_size: int,
    letter: str | None = None,
    text_color: Color | None = None,
) -> None:
    """
    Draw one hexagonal piece tile: fill, centred letter, and outline.

    Shared by the board stacks, the palette, and the drag preview so all three
    render pieces identically.
    """
    draw_poly(_vec(position), HEX_SIDES, size, 0.0, piece_color(piece))

    glyph = piece_letter(piece) if letter is None else letter
    if text_color is None:
        text_color = WHITE if piece.owner is Player.White else BLACK
    draw_text_centered(glyph, position, font_size, text_color)

    _draw_hex_outline(position, DARKGRAY, OUTLINE_THICKNESS, size)


def _draw_tile_outline(
    tile: TilePointer,
    center: Point,
    color: Color,
) -> None:
    _draw_hex_outline(
        hex_to_pixel(tile, center),
        color,
        OUTLINE_THICKNESS_SPECIAL,
    )


def _draw_hex_outline(
    center: Point,
    color: Color,
    thickness: float,
    size: float = HEX_SIZE,
) -> None:
    draw_poly_lines_ex(_vec(center), HEX_SIDES, size, 0.0, thickness, color)
