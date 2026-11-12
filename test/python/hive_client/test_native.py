"""Smoke tests for the native Hive engine binding."""

from __future__ import annotations

import hive_native
import pytest

PLACE = 0
MOVE = 1
QUEEN = 0

WHITE = 1
BLACK = 0


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

    assert isinstance(hive_native.IllegalMoveError("x"), hive_native.HiveError)
