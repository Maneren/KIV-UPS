"""Smoke tests for the native Hive engine binding and its Python facade."""

from __future__ import annotations

import hive_native
import pytest
from hive_client.domain import (
    EmptyTileError,
    GameResult,
    GameSession,
    MovePiece,
    Piece,
    PieceKind,
    Player,
    TilePointer,
    _convert,
    make_placement,
)

PLACE = 0
MOVE = 1
QUEEN = 0

WHITE = 1
BLACK = 0

ORIGIN = TilePointer(0, 0)


def test_opening_placement() -> None:
    game = hive_native.Game()

    assert game.current_player == WHITE
    assert game.result == 0
    assert game.winner is None
    assert game.is_empty_all()
    assert game.placement_targets() == [(0, 0)]
    assert game.legal_moves() == []

    assert game.submit((PLACE, 0, 0, 0, 0, QUEEN))

    assert game.current_player == BLACK
    assert not game.is_empty_all()
    assert game.stacks() == [(0, 0, [(QUEEN, WHITE)])]
    assert game.top(0, 0) == (QUEEN, WHITE)
    assert game.top(4, 4) is None
    assert game.remaining(WHITE)[QUEEN] == 0
    assert game.queen_position(WHITE) == (0, 0)


def test_follow_up_placement_and_illegal_move() -> None:
    game = hive_native.Game()
    game.apply((PLACE, 0, 0, 0, 0, QUEEN))

    assert game.can_place_at(1, 0)
    assert game.submit((PLACE, 0, 0, 1, 0, QUEEN))
    assert game.current_player == WHITE

    assert not game.submit((PLACE, 0, 0, 10, 10, QUEEN))
    assert game.current_player == WHITE


def test_invalid_records_raise_value_error() -> None:
    game = hive_native.Game()

    with pytest.raises(ValueError):
        game.is_legal((PLACE, 0, 0))
    with pytest.raises(ValueError):
        game.is_legal((2, 0, 0, 0, 0, QUEEN))
    with pytest.raises(ValueError):
        game.remaining(2)


def test_board_errors_propagate() -> None:
    game = hive_native.Game()

    with pytest.raises(hive_native.IllegalMoveError):
        game.apply((MOVE, 5, 5, 0, 0, QUEEN))

    assert issubclass(hive_native.HiveError, RuntimeError)
    assert isinstance(hive_native.IllegalMoveError("x"), hive_native.HiveError)


def test_facade_enum_parity() -> None:
    assert [kind.value for kind in PieceKind] == list(range(5))
    assert Player.Black.value == BLACK
    assert Player.White.value == WHITE
    assert [result.value for result in GameResult] == list(range(4))

    placement = make_placement(ORIGIN, PieceKind.Queen)
    assert _convert.move_to_record(placement) == (
        _convert.PLACE,
        0,
        0,
        0,
        0,
        QUEEN,
    )
    assert _convert.move_from_record(_convert.move_to_record(placement)) == placement

    movement = MovePiece(
        from_=ORIGIN,
        to=TilePointer(1, 0),
        piece_kind=PieceKind.Ant,
    )
    assert _convert.move_to_record(movement) == (
        _convert.MOVE,
        0,
        0,
        1,
        0,
        4,
    )
    assert _convert.move_from_record(_convert.move_to_record(movement)) == movement


def test_facade_opening_and_submit() -> None:
    session = GameSession()

    assert session.current_player is Player.White
    assert session.result is GameResult.ONGOING
    assert session.winner is None
    assert session.board.is_empty_all()
    assert session.placement_targets() == (ORIGIN,)

    assert session.submit(make_placement(ORIGIN, PieceKind.Queen))

    assert session.current_player is Player.Black
    assert session.board.get_top(ORIGIN) == Piece(
        kind=PieceKind.Queen, owner=Player.White
    )
    assert session.board.occupied_tiles() == {ORIGIN}
    assert len(session.board.tiles_around_hive()) == 6
    assert session.board.remaining(Player.White)[PieceKind.Queen] == 0
    assert session.queen_position(Player.White) == ORIGIN


def test_facade_board_view_raises_for_empty_tile() -> None:
    session = GameSession()

    assert session.board.is_empty(ORIGIN)
    with pytest.raises(EmptyTileError):
        session.board.get_top(ORIGIN)


def test_facade_rejects_illegal_placement() -> None:
    session = GameSession()

    assert not session.submit(make_placement(TilePointer(1, 0), PieceKind.Queen))
    assert session.current_player is Player.White
    assert session.board.is_empty_all()


def test_facade_converts_moves() -> None:
    session = GameSession()
    session.apply(make_placement(ORIGIN, PieceKind.Queen))
    session.apply(make_placement(TilePointer(1, 0), PieceKind.Queen))

    moves = session.moves_from(ORIGIN)
    assert moves
    assert all(isinstance(move, MovePiece) for move in moves)

    move = moves[0]
    assert isinstance(move, MovePiece)
    assert session.is_legal(move)
    assert session.submit(move)
    assert session.current_player is Player.Black
    assert session.board.get_top(move.to) == Piece(
        kind=PieceKind.Queen, owner=Player.White
    )
