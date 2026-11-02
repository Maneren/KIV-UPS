"""
Raylib-based GUI for the Hive client.

Renders the hex board (recentered on resize), pieces, available-piece
counters and turn indicator, and translates mouse input into game moves.
A short tutorial is drawn on the right side of the window.
"""

from __future__ import annotations

import math
from dataclasses import dataclass

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
    set_config_flags,
    set_target_fps,
    window_should_close,
)
from raylib import MOUSE_BUTTON_LEFT

from hive_client.game import (
    Board,
    Move,
    MovePiece,
    Piece,
    PieceKind,
    Player,
    TilePointer,
    make_placement,
)

__all__ = ["run"]

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
TUTORIAL_PANEL_WIDTH = 320.0
TUTORIAL_X_OFFSET = 16.0
TUTORIAL_Y_OFFSET = 100.0

PIECE_COLORS = {
    "Queen": GOLD,
    "Spider": RED,
    "Ant": BLUE,
    "Beetle": PURPLE,
    "Grasshopper": DARKGREEN,
}

PIECE_LETTERS = {
    "Queen": "Q",
    "Spider": "S",
    "Ant": "A",
    "Beetle": "B",
    "Grasshopper": "G",
}

TUTORIAL_LINES = [
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


@dataclass
class GuiState:
    window_size: Vector2
    window_center: Vector2
    selected_tile: TilePointer | None = None
    selected_kind: PieceKind = PieceKind.Queen
    valid_moves: list[Move] | None = None
    tutorial_visible: bool = True


def run(
    host: str = "127.0.0.1",
    port: int = 8080,
    width: int = 800,
    height: int = 600,
) -> None:
    """Open the client window and run the render loop."""
    set_config_flags(ConfigFlags.FLAG_MSAA_4X_HINT | ConfigFlags.FLAG_WINDOW_RESIZABLE)
    init_window(width, height, f"Hive Game ({host}:{port})")
    set_target_fps(60)

    board = Board()
    current_player = Player.White
    gui = GuiState(
        window_size=Vector2(float(width), float(height)),
        window_center=Vector2(float(width) * 0.5, float(height) * 0.5),
    )

    while not window_should_close():
        # Keep the board centered when the window is resized.
        gui.window_size = Vector2(float(get_screen_width()), float(get_screen_height()))
        gui.window_center = Vector2(gui.window_size.x * 0.5, gui.window_size.y * 0.5)

        mouse = get_mouse_position()

        # Forced pass: once the game has started, if current player has no
        # legal move or placement (e.g. all their pieces are covered by
        # beetles), skip their turn. On the empty board the first piece is
        # placed by the GUI instead, so no pass there.
        if not board.is_empty_all() and not list(
            board.moves_for_player(
                current_player, board.get_player_pieces()[current_player]
            )
        ):
            current_player = current_player.other()
            continue

        # Select which piece kind to place next.
        for key, kind in PIECE_SELECT_KEYS.items():
            if is_key_pressed(key):
                gui.selected_kind = kind

        if is_key_pressed(TUTORIAL_TOGGLE_KEY):
            gui.tutorial_visible = not gui.tutorial_visible

        if is_mouse_button_pressed(MOUSE_BUTTON_LEFT):
            new_player = _handle_input(board, gui, current_player, mouse)
            if new_player is not None:
                current_player = new_player

        begin_drawing()
        clear_background(RAYWHITE)

        if gui.tutorial_visible:
            _draw_tutorial()
        _draw_turn_label(current_player, gui.window_center.x)
        _draw_tiles(board, gui)
        _draw_available(board, gui, current_player)

        end_drawing()

    close_window()


def _handle_input(
    board: Board, gui: GuiState, current_player: Player, mouse: Vector2
) -> Player | None:
    # Empty board: first piece always goes at (0, 0).
    if board.is_empty_all():
        board.apply_move(
            make_placement(TilePointer(0, 0), gui.selected_kind), current_player
        )
        return current_player.other()

    ptr = _find_clicked_tile(board, gui, mouse)
    if ptr is None:
        return None

    # If moves are highlighted and we click a move target, execute it.
    if gui.valid_moves is not None:
        result = _execute_highlighted_move(board, gui, ptr, current_player)
        if result is not None:
            return result

    # Empty tile -> try to place the selected kind.
    if board.is_empty(ptr):
        return _try_place_piece(board, gui, ptr, current_player)

    # Occupied tile belonging to us -> select it and show valid moves.
    return _select_or_deselect_tile(board, gui, ptr, current_player)


def _find_clicked_tile(
    board: Board, gui: GuiState, mouse: Vector2
) -> TilePointer | None:
    clickable: list[TilePointer] = []
    for ptr, _ in board.pieces():
        clickable.append(ptr)
    clickable.extend(board.tiles_around_hive())

    for ptr in clickable:
        pos = hex_to_pixel(ptr, gui.window_center)
        if check_collision_point_circle(mouse, pos, HEX_SIZE):
            return ptr
    return None


def _execute_highlighted_move(
    board: Board, gui: GuiState, ptr: TilePointer, current_player: Player
) -> Player | None:
    if gui.valid_moves is None:
        return None
    if gui.selected_tile == ptr:
        gui.selected_tile = None
        gui.valid_moves = None
        return None

    for move in gui.valid_moves:
        if isinstance(move, MovePiece) and move.to == ptr:
            board.apply_move(move, current_player)
            gui.selected_tile = None
            gui.valid_moves = None
            return current_player.other()
    return None


def _try_place_piece(
    board: Board, gui: GuiState, ptr: TilePointer, current_player: Player
) -> Player | None:
    kind = gui.selected_kind
    if board.can_player_place(current_player, kind) and board.can_player_place_at(
        current_player, ptr
    ):
        board.apply_move(make_placement(ptr, kind), current_player)
        return current_player.other()
    return None


def _select_or_deselect_tile(
    board: Board, gui: GuiState, ptr: TilePointer, current_player: Player
) -> Player | None:
    top = board.get_top(ptr)
    if top.owner is current_player and board.can_player_move(current_player, ptr):
        gui.selected_tile = ptr
        gui.valid_moves = list(board.moves_for_piece(ptr, top))
    elif gui.selected_tile == ptr:
        gui.selected_tile = None
        gui.valid_moves = None
    return None


def _draw_tutorial() -> None:
    """Draw the short tutorial panel on the right side (toggleable with H)."""
    screen_width = float(get_screen_width())
    panel_width = TUTORIAL_PANEL_WIDTH
    panel_x = screen_width - panel_width
    panel_height = float(get_screen_height())
    draw_rectangle(
        int(panel_x), 0, int(panel_width), int(panel_height), Color(240, 240, 240, 255)
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


def _draw_turn_label(current_player: Player, center_x: float) -> None:
    turn_text = f"{current_player.name}'s turn"
    label_width = len(turn_text) * TEXT_FONT_SIZE * 0.55
    label_x = center_x - label_width / 2.0
    draw_text(turn_text, int(label_x), 10, TEXT_FONT_SIZE, DARKGRAY)


def _piece_color(piece: Piece) -> Color:
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


def _draw_tiles(board: Board, gui: GuiState) -> None:
    for ptr, _piece in board.pieces():
        pos = hex_to_pixel(ptr, gui.window_center)
        stack = board.get(ptr)

        for i, stack_piece in enumerate(stack):
            size = HEX_SIZE * math.pow(0.85, i)
            tile_color = _piece_color(stack_piece)
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

    # Highlight selected tile.
    if gui.selected_tile is not None:
        pos = hex_to_pixel(gui.selected_tile, gui.window_center)
        draw_hex_outline(pos, BLACK, OUTLINE_THICKNESS_SPECIAL)


def _draw_available(board: Board, gui: GuiState, current_player: Player) -> None:
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
