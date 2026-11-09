"""
Headless tests for the application controller.

These tests intentionally import no raylib modules: the controller and view
model are the UI-independent seam between game rules and rendering.
"""

from hive_client.app.controller import GameController
from hive_client.domain.game import ORIGIN, GameSession
from hive_client.domain.geometry import TilePointer
from hive_client.domain.moves import make_placement
from hive_client.domain.pieces import DEFAULT_PIECES_PER_PLAYER, PieceKind, Player


def _session_with_white_queen_and_black_ant() -> GameSession:
    session = GameSession()
    # ``apply`` is the low-level transition used for setting up fixtures.
    session.apply(make_placement(ORIGIN, PieceKind.Queen))
    session.apply(make_placement(TilePointer(1, 0), PieceKind.Ant))
    return session


def test_opening_placement_only_at_origin() -> None:
    """The opening piece is only accepted at the board centre."""
    controller = GameController()

    assert controller.begin_palette_drag(PieceKind.Queen)
    assert not controller.drop_on_tile(TilePointer(1, 0))
    assert controller.session.board.is_empty_all()

    assert controller.begin_palette_drag(PieceKind.Queen)
    assert controller.drop_on_tile(ORIGIN)
    assert controller.session.current_player is Player.Black


def test_hover_own_piece_exposes_moves() -> None:
    """Hovering an own movable piece previews its moves."""
    controller = GameController(_session_with_white_queen_and_black_ant())

    controller.hover(tile=ORIGIN)

    assert controller.state.hovered_tile == ORIGIN
    assert controller.state.hover_targets
    view = controller.view()
    assert view.hovered_tile == ORIGIN
    assert view.hover_targets
    assert view.hint is None


def test_hover_own_piece_before_queen_shows_hint() -> None:
    """A piece cannot move before the Queen is placed, so show a hint."""
    session = GameSession()
    session.apply(make_placement(ORIGIN, PieceKind.Ant))
    session.apply(make_placement(TilePointer(1, 0), PieceKind.Ant))
    controller = GameController(session)

    controller.hover(tile=ORIGIN)

    view = controller.view()
    assert not view.hover_targets
    assert view.hint == "Place your Queen before moving pieces."


def test_hover_opponent_piece_exposes_no_moves() -> None:
    """Hovering a piece that is not the current player's shows no targets."""
    controller = GameController(_session_with_white_queen_and_black_ant())

    controller.hover(tile=TilePointer(1, 0))

    assert not controller.state.hover_targets
    assert not controller.view().hover_targets


def test_hover_palette_exposes_placement_targets() -> None:
    """Hovering a palette piece previews where it could be placed."""
    controller = GameController()

    controller.hover(kind=PieceKind.Ant)

    assert controller.state.hovered_kind == PieceKind.Ant
    assert controller.view().hover_targets == frozenset({ORIGIN})


def test_palette_drag_places_first_piece() -> None:
    """Dragging from the palette can perform the opening placement."""
    controller = GameController()

    assert controller.begin_palette_drag(PieceKind.Queen)
    assert controller.state.drag is not None
    assert controller.state.drag.targets == frozenset({ORIGIN})
    assert controller.drop_on_tile(ORIGIN)

    assert controller.session.board.get_top(ORIGIN).kind is PieceKind.Queen
    assert controller.state.drag is None


def test_palette_drag_rejects_exhausted_piece_kind() -> None:
    """A kind with no supply left cannot start a palette drag."""
    controller = GameController()
    controller.session.board._player_pieces[Player.White][PieceKind.Queen] = 0

    assert not controller.begin_palette_drag(PieceKind.Queen)


def test_palette_drag_ignores_illegal_drop() -> None:
    """Dropping outside the legal targets changes nothing."""
    controller = GameController()
    assert controller.begin_palette_drag(PieceKind.Queen)

    assert not controller.drop_on_tile(TilePointer(4, -2))
    assert controller.session.board.is_empty_all()


def test_board_drag_moves_piece() -> None:
    """Dragging an existing piece to a target executes the move."""
    controller = GameController(_session_with_white_queen_and_black_ant())

    assert controller.begin_board_drag(ORIGIN)
    assert controller.state.drag is not None
    target = next(iter(controller.state.drag.targets))
    assert controller.drop_on_tile(target)

    assert controller.session.current_player is Player.Black
    assert controller.session.board.get_top(target).kind is PieceKind.Queen


def test_advance_passes_when_player_has_no_legal_action() -> None:
    """A player with no legal action automatically passes."""
    session = GameSession()
    # Only a Black piece is on the board; White cannot place next to it and
    # has no queen to move.
    session.board.apply_move(make_placement(ORIGIN, PieceKind.Ant), Player.Black)
    controller = GameController(session)

    controller.advance()

    assert session.current_player is Player.Black


def test_view_reports_remaining_pieces_for_current_player() -> None:
    """The view model reports supplies for the player to move."""
    controller = GameController()
    view = controller.view()

    counts = view.remaining_map
    assert counts[PieceKind.Queen] == DEFAULT_PIECES_PER_PLAYER[PieceKind.Queen]
    assert counts[PieceKind.Spider] == DEFAULT_PIECES_PER_PLAYER[PieceKind.Spider]
    assert view.current_player is Player.White
