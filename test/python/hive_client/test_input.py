"""Headless tests for mouse-to-intent routing (raylib is not imported)."""

from hive_client.app.controller import GameController
from hive_client.domain.game import ORIGIN, GameSession
from hive_client.domain.geometry import TilePointer
from hive_client.domain.moves import make_placement
from hive_client.domain.pieces import PieceKind, Player
from hive_client.ui.geometry import hex_to_pixel
from hive_client.ui.input import InputRouter, MouseEvent
from hive_client.ui.layout import palette_tiles

CENTER = (400.0, 300.0)


def _session_with_white_queen_and_black_ant() -> GameSession:
    session = GameSession()
    session.apply(make_placement(ORIGIN, PieceKind.Queen))
    session.apply(make_placement(TilePointer(1, 0), PieceKind.Ant))
    return session


def _click(router: InputRouter, point: tuple[float, float]) -> None:
    router.handle(MouseEvent(pressed=True, released=False, position=point), CENTER)
    router.handle(MouseEvent(pressed=False, released=True, position=point), CENTER)


def test_click_places_opening_piece() -> None:
    """A press/release on the centre hex performs the opening placement."""
    controller = GameController()
    router = InputRouter(controller)

    _click(router, hex_to_pixel(ORIGIN, CENTER))

    assert controller.session.board.get_top(ORIGIN).kind is PieceKind.Queen
    assert controller.session.current_player is Player.Black


def test_click_selects_a_piece_and_then_moves_it() -> None:
    """Clicking a piece selects it; clicking a target executes the move."""
    controller = GameController(_session_with_white_queen_and_black_ant())
    router = InputRouter(controller)

    _click(router, hex_to_pixel(ORIGIN, CENTER))
    assert controller.state.selected_tile == ORIGIN

    target = next(iter(controller.view().move_targets))
    _click(router, hex_to_pixel(target, CENTER))

    assert controller.session.board.get_top(target).kind is PieceKind.Queen
    assert controller.session.current_player is Player.Black


def test_palette_click_selects_kind_without_placing() -> None:
    """A click on the palette only changes the selected kind."""
    controller = GameController()
    router = InputRouter(controller)
    kind, rect = next(
        (kind, rect) for kind, rect in palette_tiles() if kind is PieceKind.Ant
    )

    _click(router, rect.center)

    assert controller.state.selected_kind is PieceKind.Ant
    assert controller.session.board.is_empty_all()
    assert controller.state.drag is None


def test_palette_drag_to_board_places_piece() -> None:
    """Pressing the palette and releasing on a target places the piece."""
    controller = GameController()
    router = InputRouter(controller)
    _kind, rect = next(
        (kind, rect) for kind, rect in palette_tiles() if kind is PieceKind.Ant
    )

    router.handle(
        MouseEvent(pressed=True, released=False, position=rect.center),
        CENTER,
    )
    router.handle(
        MouseEvent(pressed=False, released=True, position=hex_to_pixel(ORIGIN, CENTER)),
        CENTER,
    )

    assert controller.session.board.get_top(ORIGIN).kind is PieceKind.Ant
    assert controller.state.drag is None


def test_click_far_from_hive_is_ignored() -> None:
    """Clicks outside the playable area neither place nor select anything."""
    controller = GameController(_session_with_white_queen_and_black_ant())
    router = InputRouter(controller)

    _click(router, hex_to_pixel(TilePointer(20, -10), CENTER))

    assert controller.state.selected_tile is None
    assert controller.session.current_player is Player.White
