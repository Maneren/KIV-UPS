"""
Raylib-based GUI for the Hive client.

MVC split:
- HexRenderer: all drawing (board, pieces, highlights, tutorial, labels).
- InputController: mouse/keyboard -> GameState mutations.
- HiveGUI: owns GameState, runs the raylib loop, coordinates renderer and
  controller.

A short tutorial is drawn on the right side of the window.
"""

from __future__ import annotations

import math
from dataclasses import dataclass, field

from pyray import (
    BLACK,
    BLUE,
    DARKGRAY,
    DARKGREEN,
    GOLD,
    GREEN,
    PURPLE,
    RAYWHITE,
    RED,
    WHITE,
    Color,
    ConfigFlags,
    Rectangle,
    Vector2,
    begin_drawing,
    check_collision_point_circle,
    clear_background,
    close_window,
    draw_line_ex,
    draw_poly,
    draw_rectangle,
    draw_text,
    end_drawing,
    get_mouse_position,
    get_screen_height,
    get_screen_width,
    init_window,
    is_key_pressed,
    is_mouse_button_pressed,
    is_mouse_button_released,
    set_config_flags,
    set_target_fps,
    window_should_close,
)
from raylib import MOUSE_BUTTON_LEFT

from hive_client.game import (
    Board,
    GameState,
    Move,
    MovePiece,
    Piece,
    PieceKind,
    Player,
    TilePointer,
    make_placement,
)

__all__ = ["HiveGUI", "run"]

HEX_SIZE = 40.0
HEX_SIDES = 6
HEX_ANGLE_STEP = math.pi / 3.0
HEX_X_FACTOR = 1.5
HEX_Y_FACTOR = math.sqrt(3.0)
OUTLINE_THICKNESS = 2.0
OUTLINE_THICKNESS_SPECIAL = 3.0
TEXT_FONT_SIZE = 24
TUTORIAL_FONT_SIZE = 16
TUTORIAL_TOGGLE_KEY = 72  # 'H'
PIECE_SELECT_KEYS = {
    81: PieceKind.Queen,  # 'Q'
    83: PieceKind.Spider,  # 'S'
    66: PieceKind.Beetle,  # 'B'
    71: PieceKind.Grasshopper,  # 'G'
    65: PieceKind.Ant,  # 'A'
}
AVAILABLE_X_OFFSET = 20.0
AVAILABLE_Y_OFFSET = 20.0
PALETTE_X_OFFSET = 20.0
PALETTE_Y_OFFSET = 130.0
PALETTE_TILE_SIZE = 34.0
PALETTE_ROW_HEIGHT = 46.0
TUTORIAL_PANEL_WIDTH = 320.0
TUTORIAL_X_OFFSET = 16.0
TUTORIAL_Y_OFFSET = 100.0
# Collision radius for board hex click detection (smaller than HEX_SIZE to
# avoid flickering between adjacent overlapping hexes).
BOARD_CLICK_RADIUS = HEX_SIZE * 0.6

PIECE_COLORS: dict[str, Color] = {
    "Queen": GOLD,
    "Spider": RED,
    "Ant": BLUE,
    "Beetle": PURPLE,
    "Grasshopper": DARKGREEN,
}

PIECE_LETTERS: dict[str, str] = {
    "Queen": "Q",
    "Spider": "S",
    "Ant": "A",
    "Beetle": "B",
    "Grasshopper": "G",
}

TUTORIAL_LINES: list[str] = [
    "Hive — how to play",
    "",
    "• Click an empty hex to place",
    "  your selected piece.",
    "  The first piece must go at",
    "  the board center (0, 0).",
    "",
    "• Click your own piece to",
    "  select it and show its valid",
    "  moves (green rings).",
    "",
    "• Click a green ring to move",
    "  the piece there.",
    "",
    "• Queen moves 1 step.",
    "• Spider moves exactly 3 steps.",
    "• Beetle moves onto any",
    "  neighbouring occupied cell.",
    "• Grasshopper jumps over a line",
    "  of pieces.",
    "• Ant moves any number of steps",
    "  around the hive.",
    "",
    "Goal: surround the enemy queen",
    "with pieces (any colour).",
]


def hex_to_pixel(ptr: TilePointer, center: Vector2) -> Vector2:
    """Convert axial (p, q) coords to screen pixels (pointy-top hexes)."""
    x = HEX_X_FACTOR * ptr.p
    y = HEX_Y_FACTOR * (ptr.q + ptr.p / 2.0)
    return Vector2(center.x + x * HEX_SIZE, center.y + y * HEX_SIZE)


def piece_letter(piece: Piece) -> str:
    letter = PIECE_LETTERS[piece.kind.name]
    if piece.owner is Player.Black:
        return letter.lower()
    return letter


def draw_hex_outline(
    center: Vector2,
    color: Color,
    thickness: float,
    size: float = HEX_SIZE,
) -> None:
    for i in range(HEX_SIDES):
        start_angle = i * HEX_ANGLE_STEP
        end_angle = ((i + 1) % HEX_SIDES) * HEX_ANGLE_STEP
        start = Vector2(
            center.x + size * math.cos(start_angle),
            center.y + size * math.sin(start_angle),
        )
        end = Vector2(
            center.x + size * math.cos(end_angle),
            center.y + size * math.sin(end_angle),
        )
        draw_line_ex(start, end, thickness, color)


def snap_to_hex(mouse: Vector2, center: Vector2) -> Vector2:
    """Snap a screen position to the nearest hex grid center."""
    dx = mouse.x - center.x
    dy = mouse.y - center.y
    p = dx / (HEX_X_FACTOR * HEX_SIZE)
    q = dy / (HEX_Y_FACTOR * HEX_SIZE) - p / 2.0
    p_rounded = round(p)
    q_rounded = round(q)
    snapped_x = center.x + HEX_X_FACTOR * p_rounded * HEX_SIZE
    snapped_y = center.y + HEX_Y_FACTOR * (q_rounded + p_rounded / 2.0) * HEX_SIZE
    return Vector2(snapped_x, snapped_y)


def _palette_tiles() -> list[tuple[PieceKind, Rectangle]]:
    """Bounding rects for each draggable piece tile in the palette."""
    rects: list[tuple[PieceKind, Rectangle]] = []
    piece_kinds = (
        PieceKind.Queen,
        PieceKind.Spider,
        PieceKind.Beetle,
        PieceKind.Ant,
        PieceKind.Grasshopper,
    )
    for i, kind in enumerate(piece_kinds):
        y = PALETTE_Y_OFFSET + i * PALETTE_ROW_HEIGHT
        rects.append(
            (
                kind,
                Rectangle(
                    PALETTE_X_OFFSET,
                    y,
                    PALETTE_TILE_SIZE,
                    PALETTE_TILE_SIZE,
                ),
            )
        )
    return rects


@dataclass
class MouseEvent:
    """Mouse event state for a single frame."""

    pressed: bool
    released: bool
    position: Vector2


@dataclass
class GuiState:
    """Transient UI state owned by HiveGUI."""

    window_size: Vector2
    window_center: Vector2
    selected_tile: TilePointer | None = None
    selected_kind: PieceKind = PieceKind.Queen
    valid_moves: list[Move] | None = None
    tutorial_visible: bool = True
    dragging_kind: PieceKind | None = None
    dragging_from: TilePointer | None = None
    drag_targets: list[TilePointer] = field(default_factory=list)


class HexRenderer:
    """Pure rendering: board, pieces, highlights, tutorial, labels."""

    def __init__(self) -> None:
        self.background = Color(240, 240, 240, 255)

    def draw(
        self,
        gui: GuiState,
        game: GameState,
        screen_width: float,
    ) -> None:
        begin_drawing()
        clear_background(RAYWHITE)

        if gui.tutorial_visible:
            self._draw_tutorial(screen_width)
        self._draw_turn_label(game.current_player, gui.window_center.x)
        self._draw_tiles(game.board, gui)
        self._draw_available(game.board, gui, game.current_player)
        self._draw_palette(gui, game.board, game.current_player)

        # Highlight drag targets while dragging.
        if gui.dragging_kind is not None:
            for ptr in gui.drag_targets:
                pos = hex_to_pixel(ptr, gui.window_center)
                draw_hex_outline(pos, GREEN, OUTLINE_THICKNESS_SPECIAL)
            mouse = get_mouse_position()
            snapped = snap_to_hex(mouse, gui.window_center)
            self._draw_drag_piece(snapped, gui.dragging_kind, game.current_player)
        elif gui.dragging_from is not None:
            for ptr in gui.drag_targets:
                pos = hex_to_pixel(ptr, gui.window_center)
                draw_hex_outline(pos, GREEN, OUTLINE_THICKNESS_SPECIAL)
            mouse = get_mouse_position()
            snapped = snap_to_hex(mouse, gui.window_center)
            top = game.board.get_top(gui.dragging_from)
            self._draw_drag_piece(snapped, top.kind, top.owner)

        end_drawing()

    def _draw_tutorial(self, screen_width: float) -> None:
        panel_width = TUTORIAL_PANEL_WIDTH
        panel_x = screen_width - panel_width
        panel_height = float(get_screen_height())
        draw_rectangle(
            int(panel_x), 0, int(panel_width), int(panel_height), self.background
        )
        draw_line_ex(
            Vector2(panel_x, 0),
            Vector2(panel_x, panel_height),
            2.0,
            DARKGRAY,
        )

        y = TUTORIAL_Y_OFFSET
        for line in TUTORIAL_LINES:
            draw_text(
                line,
                int(panel_x + TUTORIAL_X_OFFSET),
                int(y),
                TUTORIAL_FONT_SIZE,
                DARKGRAY,
            )
            y += TUTORIAL_FONT_SIZE + 4

    def _draw_turn_label(self, current_player: Player, center_x: float) -> None:
        turn_text = f"{current_player.name}'s turn"
        label_width = len(turn_text) * TEXT_FONT_SIZE * 0.55
        label_x = center_x - label_width / 2.0
        draw_text(turn_text, int(label_x), 10, TEXT_FONT_SIZE, DARKGRAY)

    def _draw_tiles(self, board: Board, gui: GuiState) -> None:
        for ptr, _piece in board.pieces():
            pos = hex_to_pixel(ptr, gui.window_center)
            stack = board.get(ptr)

            for i, stack_piece in enumerate(stack):
                # Skip the top piece at dragging_from (it's being dragged).
                if ptr == gui.dragging_from and i == len(stack) - 1:
                    continue
                size = HEX_SIZE * math.pow(0.85, i)
                tile_color = self._piece_color(stack_piece)
                draw_poly(
                    Vector2(int(pos.x), int(pos.y)),
                    HEX_SIDES,
                    size,
                    0.0,
                    tile_color,
                )

                text_color = WHITE if stack_piece.owner is Player.White else BLACK
                letter = piece_letter(stack_piece)
                draw_text(
                    letter,
                    int(pos.x) - 6,
                    int(pos.y) - 8,
                    TEXT_FONT_SIZE,
                    text_color,
                )
                draw_hex_outline(pos, DARKGRAY, OUTLINE_THICKNESS, size)

        # Highlight valid move targets.
        if gui.valid_moves:
            for move in gui.valid_moves:
                if isinstance(move, MovePiece):
                    pos = hex_to_pixel(move.to, gui.window_center)
                    draw_hex_outline(pos, GREEN, OUTLINE_THICKNESS_SPECIAL)

        # Highlight tile being dragged from board.
        if gui.dragging_from is not None:
            pos = hex_to_pixel(gui.dragging_from, gui.window_center)
            draw_hex_outline(pos, BLUE, OUTLINE_THICKNESS_SPECIAL)

        # Highlight selected tile.
        if gui.selected_tile is not None:
            pos = hex_to_pixel(gui.selected_tile, gui.window_center)
            draw_hex_outline(pos, BLACK, OUTLINE_THICKNESS_SPECIAL)

    def _draw_available(
        self,
        board: Board,
        gui: GuiState,
        current_player: Player,
    ) -> None:
        available = board.get_player_pieces()[current_player]
        y = AVAILABLE_Y_OFFSET
        draw_text(
            f"Selected: {gui.selected_kind.name[0]}",
            int(AVAILABLE_X_OFFSET),
            int(y),
            TEXT_FONT_SIZE,
            DARKGRAY,
        )
        y += TEXT_FONT_SIZE + 4
        for piece_kind, count in available.items():
            if count == 0:
                continue
            letter = PIECE_LETTERS[piece_kind.name]
            draw_text(
                f"{letter}: {count}",
                int(AVAILABLE_X_OFFSET),
                int(y),
                TEXT_FONT_SIZE,
                DARKGRAY,
            )
            y += TEXT_FONT_SIZE + 4

    def _draw_palette(
        self, gui: GuiState, board: Board, current_player: Player
    ) -> None:
        """Draw draggable piece tiles with remaining counts."""
        available = board.get_player_pieces()[current_player]
        for kind, rect in _palette_tiles():
            center_x = rect.x + PALETTE_TILE_SIZE / 2
            center_y = rect.y + PALETTE_TILE_SIZE / 2
            is_dragging = gui.dragging_kind is kind
            color = self._piece_color(Piece(kind=kind, owner=current_player))

            # Tile background as hexagon.
            draw_poly(
                Vector2(center_x, center_y),
                HEX_SIDES,
                PALETTE_TILE_SIZE / 2,
                0.0,
                color,
            )
            draw_hex_outline(
                Vector2(center_x, center_y),
                DARKGRAY,
                OUTLINE_THICKNESS,
                PALETTE_TILE_SIZE / 2,
            )

            # Piece letter.
            letter = PIECE_LETTERS[kind.name]
            text_color = WHITE if current_player is Player.White else BLACK
            draw_text(
                letter,
                int(center_x - 8),
                int(center_y - 10),
                int(PALETTE_TILE_SIZE * 0.8),
                text_color,
            )

            # Count next to tile.
            count = available.get(kind, 0)
            draw_text(
                f": {count}",
                int(rect.x + PALETTE_TILE_SIZE + 6),
                int(center_y - 8),
                TEXT_FONT_SIZE,
                DARKGRAY,
            )

            if is_dragging:
                draw_hex_outline(
                    Vector2(center_x, center_y),
                    GREEN,
                    OUTLINE_THICKNESS_SPECIAL,
                    PALETTE_TILE_SIZE / 2,
                )

    def _draw_drag_piece(self, mouse: Vector2, kind: PieceKind, owner: Player) -> None:
        """Draw the piece being dragged, following the mouse."""
        color = self._piece_color(Piece(kind=kind, owner=owner))
        draw_poly(mouse, HEX_SIDES, HEX_SIZE * 0.9, 0.0, color)
        letter = PIECE_LETTERS[kind.name]
        draw_text(
            letter,
            int(mouse.x) - 6,
            int(mouse.y) - 8,
            int(TEXT_FONT_SIZE * 0.9),
            WHITE,
        )
        draw_hex_outline(mouse, DARKGRAY, OUTLINE_THICKNESS, HEX_SIZE * 0.9)

    def _piece_color(self, piece: Piece) -> Color:
        """Return the piece color: base kind color for White, darkened for Black."""
        base = PIECE_COLORS[piece.kind.name]
        if hasattr(base, "r"):
            r, g, b, a = base.r, base.g, base.b, base.a
        else:
            r, g, b, a = base
        if piece.owner is Player.Black:
            return Color(
                max(0, int(r * 0.5)),
                max(0, int(g * 0.5)),
                max(0, int(b * 0.5)),
                a,
            )
        return Color(r, g, b, a)


class InputController:
    """Mouse/keyboard input -> GameState mutations."""

    def __init__(self) -> None:
        pass

    def handle_input(
        self,
        game: GameState,
        gui: GuiState,
        event: MouseEvent,
    ) -> None:
        mouse = event.position

        # Empty board: first piece always goes at (0, 0).
        if game.board.is_empty_all():
            if event.pressed:
                game.apply(make_placement(TilePointer(0, 0), gui.selected_kind))
            return

        palette_rects = self._palette_rects()

        # Start a drag from the piece palette.
        if event.pressed:
            self._start_drag(gui, game, mouse, palette_rects)
            return

        # Complete a drag: place/move on a legal target hex, otherwise cancel.
        is_dragging = gui.dragging_kind is not None or gui.dragging_from is not None
        if event.released and is_dragging:
            self._complete_drag(game, gui, mouse)
            return

        # While dragging, highlight targets but don't process board clicks.
        if gui.dragging_kind is not None or gui.dragging_from is not None:
            return

        # Board clicks only happen on press; hovering should only update
        # selection/highlights visually without executing moves.
        if not event.pressed:
            return

        self._handle_board_click(game, gui, mouse)

    def _handle_board_click(
        self,
        game: GameState,
        gui: GuiState,
        mouse: Vector2,
    ) -> None:
        ptr = self._find_clicked_tile(game.board, gui, mouse)
        if ptr is None:
            return

        # If moves are highlighted and we click a move target, execute it.
        move_executed = False
        if gui.valid_moves is not None:
            move_executed = self._execute_highlighted_move(game, gui, ptr)

        if not move_executed:
            if game.board.is_empty(ptr):
                self._try_place_piece(game, gui, ptr)
            else:
                self._select_or_deselect_tile(game, gui, ptr)

    def _start_drag(
        self,
        gui: GuiState,
        game: GameState,
        mouse: Vector2,
        palette_rects: list[tuple[Rectangle, PieceKind]],
    ) -> None:
        # Check palette first.
        for rect, kind in palette_rects:
            if (
                rect.x <= mouse.x <= rect.x + rect.width
                and rect.y <= mouse.y <= rect.y + rect.height
            ):
                gui.dragging_kind = kind
                gui.drag_targets = [
                    ptr
                    for ptr in game.board.valid_placements(game.current_player)
                    if game.board.can_player_place_at(game.current_player, ptr)
                ]
                return

        # Check board: pick up own piece for dragging.
        ptr = self._find_clicked_tile(game.board, gui, mouse)
        if ptr is None:
            return
        if game.board.is_empty(ptr):
            return
        top = game.board.get_top(ptr)
        if top.owner is game.current_player:
            assert ptr is not None
            if game.board.can_player_move(game.current_player, ptr):
                gui.dragging_from = ptr
                gui.drag_targets = [
                    m.to
                    for m in game.board.moves_for_piece(ptr, top)
                    if isinstance(m, MovePiece)
                ]

    def _complete_drag(self, game: GameState, gui: GuiState, mouse: Vector2) -> None:
        kind = gui.dragging_kind
        from_ptr = gui.dragging_from
        if kind is None and from_ptr is None:
            return

        ptr = self._find_clicked_tile(game.board, gui, mouse)

        if kind is not None:
            # Dragging from palette: place new piece.
            if ptr is not None and ptr in gui.drag_targets:
                game.apply(make_placement(ptr, kind))
        elif from_ptr is not None:
            # Dragging from board: move existing piece.
            top = game.board.get_top(from_ptr)
            if ptr is not None and ptr in gui.drag_targets:
                move = MovePiece(from_=from_ptr, to=ptr, piece_kind=top.kind)
                game.apply(move)

        gui.dragging_kind = None
        gui.dragging_from = None
        gui.drag_targets = []

    def _palette_rects(self) -> list[tuple[Rectangle, PieceKind]]:
        """Bounding rects for each draggable piece tile in the palette."""
        return [(rect, kind) for kind, rect in _palette_tiles()]

    def _find_clicked_tile(
        self,
        board: Board,
        gui: GuiState,
        mouse: Vector2,
    ) -> TilePointer | None:
        clickable: list[TilePointer] = []
        for ptr, _ in board.pieces():
            clickable.append(ptr)
        clickable.extend(board.tiles_around_hive())

        # Snap to the nearest hex center so the whole tile counts as a target,
        # not just the middle.
        snapped = snap_to_hex(mouse, gui.window_center)

        for ptr in clickable:
            pos = hex_to_pixel(ptr, gui.window_center)
            if check_collision_point_circle(snapped, pos, BOARD_CLICK_RADIUS):
                return ptr
        return None

    def _execute_highlighted_move(
        self,
        game: GameState,
        gui: GuiState,
        ptr: TilePointer,
    ) -> bool:
        if gui.valid_moves is None:
            return False
        if gui.selected_tile == ptr:
            gui.selected_tile = None
            gui.valid_moves = None
            return True

        for move in gui.valid_moves:
            if isinstance(move, MovePiece) and move.to == ptr:
                game.apply(move)
                gui.selected_tile = None
                gui.valid_moves = None
                return True
        return False

    def _try_place_piece(
        self,
        game: GameState,
        gui: GuiState,
        ptr: TilePointer,
    ) -> None:
        kind = gui.selected_kind
        if game.board.can_player_place(
            game.current_player, kind
        ) and game.board.can_player_place_at(game.current_player, ptr):
            game.apply(make_placement(ptr, kind))

    def _select_or_deselect_tile(
        self,
        game: GameState,
        gui: GuiState,
        ptr: TilePointer,
    ) -> None:
        top = game.board.get_top(ptr)
        if top.owner is game.current_player and game.board.can_player_move(
            game.current_player, ptr
        ):
            gui.selected_tile = ptr
            gui.valid_moves = list(game.board.moves_for_piece(ptr, top))
        elif gui.selected_tile == ptr:
            gui.selected_tile = None
            gui.valid_moves = None


class HiveGUI:
    """Owns GameState, runs the raylib loop, coordinates renderer/controller."""

    def __init__(self, width: int = 800, height: int = 600) -> None:
        """Create the GUI with a fresh GameState and default window size."""
        self.width = width
        self.height = height
        self.game = GameState()
        self.gui = GuiState(
            window_size=Vector2(float(width), float(height)),
            window_center=Vector2(float(width) * 0.5, float(height) * 0.5),
        )
        self.renderer = HexRenderer()
        self.controller = InputController()

    def run(self) -> None:
        """Open the client window and run the render loop."""
        set_config_flags(
            ConfigFlags.FLAG_MSAA_4X_HINT | ConfigFlags.FLAG_WINDOW_RESIZABLE
        )
        init_window(self.width, self.height, "Hive Game")
        set_target_fps(60)

        while not window_should_close():
            # Keep the board centered when the window is resized.
            self.gui.window_size = Vector2(
                float(get_screen_width()), float(get_screen_height())
            )
            self.gui.window_center = Vector2(
                self.gui.window_size.x * 0.5, self.gui.window_size.y * 0.5
            )

            mouse = get_mouse_position()

            event = MouseEvent(
                pressed=is_mouse_button_pressed(MOUSE_BUTTON_LEFT),
                released=is_mouse_button_released(MOUSE_BUTTON_LEFT),
                position=mouse,
            )

            # Forced pass: once the game has started, if current player has no
            # legal move or placement (e.g. all their pieces are covered by
            # beetles), skip their turn. On the empty board the first piece is
            # placed by the GUI instead, so no pass there.
            if not self.game.board.is_empty_all() and not self.game.legal_moves():
                self.game.current_player = self.game.current_player.other()
                continue

            # Select which piece kind to place next.
            for key, kind in PIECE_SELECT_KEYS.items():
                if is_key_pressed(key):
                    self.gui.selected_kind = kind

            if is_key_pressed(TUTORIAL_TOGGLE_KEY):
                self.gui.tutorial_visible = not self.gui.tutorial_visible

            self.controller.handle_input(self.game, self.gui, event)

            self.renderer.draw(
                self.gui,
                self.game,
                float(get_screen_width()),
            )

        close_window()


def run(
    _host: str = "127.0.0.1",
    _port: int = 8080,
    width: int = 800,
    height: int = 600,
) -> None:
    """Open the client window and run the render loop."""
    # host/port are accepted for future network integration; the GUI currently
    # runs in local (hot-seat) mode.
    HiveGUI(width, height).run()
