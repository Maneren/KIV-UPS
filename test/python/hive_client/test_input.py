"""Headless tests for mouse-to-intent routing (raylib is not imported)."""

import hive_native
from hive_client.app.controller import GameController
from hive_client.domain.game import ORIGIN
from hive_client.domain.geometry import TilePointer
from hive_client.domain.moves import Move
from hive_client.domain.pieces import Piece, PieceKind, Player
from hive_client.ui.geometry import hex_to_pixel
from hive_client.ui.input import MouseEvent, MouseRouter
from hive_client.ui.layout import PALETTE_PIECES, rect_center

CENTER = (400.0, 300.0)


def _top(game: hive_native.Game, tile: TilePointer) -> Piece:
    return game.top(tile)


def _game_with_white_queen_and_black_ant() -> hive_native.Game:
    game = hive_native.Game()
    game.apply(Move.place(ORIGIN, PieceKind.Queen))
    game.apply(Move.place(TilePointer(1, 0), PieceKind.Ant))
    return game


def _press(router: MouseRouter, point: tuple[float, float]) -> None:
    router.handle(MouseEvent(pressed=True, released=False, position=point), CENTER)


def _release(router: MouseRouter, point: tuple[float, float]) -> None:
    router.handle(MouseEvent(pressed=False, released=True, position=point), CENTER)


def _move(router: MouseRouter, point: tuple[float, float]) -> None:
    router.handle(MouseEvent(pressed=False, released=False, position=point), CENTER)


def _drag(
    router: MouseRouter,
    start: tuple[float, float],
    end: tuple[float, float],
) -> None:
    _press(router, start)
    _release(router, end)


def _palette_point(kind: PieceKind) -> tuple[float, float]:
    rect = next(rect for palette_kind, rect in PALETTE_PIECES if palette_kind is kind)
    return rect_center(rect)


def test_palette_drag_places_opening_piece() -> None:
    """Dragging a palette piece to the centre performs the opening placement."""
    controller = GameController()
    router = MouseRouter(controller)

    _drag(router, _palette_point(PieceKind.Queen), hex_to_pixel(ORIGIN, CENTER))

    assert _top(controller.game, ORIGIN).kind is PieceKind.Queen
    assert controller.current_player is Player.Black
    assert controller.state.drag is None


def test_click_without_drag_does_not_place_piece() -> None:
    """A press/release without moving off the tile places nothing."""
    controller = GameController()
    router = MouseRouter(controller)

    _drag(router, hex_to_pixel(ORIGIN, CENTER), hex_to_pixel(ORIGIN, CENTER))

    assert controller.game.is_empty_all()
    assert controller.state.drag is None


def test_board_drag_moves_piece() -> None:
    """Dragging a piece to a target executes the move."""
    controller = GameController(_game_with_white_queen_and_black_ant())
    router = MouseRouter(controller)

    _press(router, hex_to_pixel(ORIGIN, CENTER))
    drag = controller.state.drag
    assert drag is not None
    target = next(iter(drag.targets))
    _release(router, hex_to_pixel(target, CENTER))

    assert controller.current_player is Player.Black
    assert _top(controller.game, target).kind is PieceKind.Queen
    assert controller.state.drag is None


def test_hover_over_own_piece_exposes_targets() -> None:
    """Hovering an own piece previews its legal moves without clicking."""
    controller = GameController(_game_with_white_queen_and_black_ant())
    router = MouseRouter(controller)

    _move(router, hex_to_pixel(ORIGIN, CENTER))

    assert controller.state.hovered_tile == ORIGIN
    assert controller.view().hover_targets


def test_hover_before_queen_shows_hint() -> None:
    """Hovering a movable piece without a Queen explains why it is blocked."""
    game = hive_native.Game()
    game.apply(Move.place(ORIGIN, PieceKind.Ant))
    game.apply(Move.place(TilePointer(1, 0), PieceKind.Ant))
    controller = GameController(game)
    router = MouseRouter(controller)

    _move(router, hex_to_pixel(ORIGIN, CENTER))

    assert controller.view().hint is not None
    assert not controller.view().hover_targets


def test_hover_over_palette_exposes_placement_targets() -> None:
    """Hovering a palette piece previews its placement tiles."""
    controller = GameController()
    router = MouseRouter(controller)

    _move(router, _palette_point(PieceKind.Ant))

    assert controller.state.hovered_kind is PieceKind.Ant
    assert controller.view().hover_targets == frozenset({ORIGIN})


def test_hover_in_palette_gap_is_ignored() -> None:
    """Hovering the gap between palette tiles must not crash."""
    controller = GameController()
    router = MouseRouter(controller)
    _kind, first = PALETTE_PIECES[0]
    gap_point = (first.x + 4, first.y + first.height + 2)

    _move(router, gap_point)

    assert not controller.view().hover_targets


def test_hover_far_from_hive_is_ignored() -> None:
    """Hovering outside the playable area neither previews nor changes anything."""
    controller = GameController(_game_with_white_queen_and_black_ant())
    router = MouseRouter(controller)

    _move(router, hex_to_pixel(TilePointer(20, -10), CENTER))

    assert not controller.view().hover_targets
    assert controller.current_player is Player.White
