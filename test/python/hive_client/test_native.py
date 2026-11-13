"""Tests for the native Hive engine binding and Python value objects."""

from __future__ import annotations

from typing import cast

import hive_native
import pytest
from hive_client.domain import (
    GameResult,
    Move,
    PieceKind,
    Player,
    TilePointer,
)
from hive_client.domain.moves import MOVE, PLACE

QUEEN = 0
WHITE = 1
BLACK = 0

ORIGIN = TilePointer(0, 0)


def test_opening_placement() -> None:
    game = hive_native.Game()

    assert game.current_player == WHITE
    assert game.result == 0
    assert game.is_empty_all()
    assert game.placement_targets() == [(0, 0)]

    assert game.submit(Move.place(ORIGIN, PieceKind.Queen))

    assert game.current_player == BLACK
    assert not game.is_empty_all()
    assert game.stacks() == [(0, 0, [(QUEEN, WHITE)])]
    assert game.top(0, 0) == (QUEEN, WHITE)
    assert game.top(4, 4) is None
    assert game.remaining(WHITE)[QUEEN] == 0
    assert game.has_placed_queen(WHITE)
    assert not game.has_placed_queen(BLACK)


def test_follow_up_placement_and_illegal_move() -> None:
    game = hive_native.Game()
    game.apply(Move.place(ORIGIN, PieceKind.Queen))

    assert (1, 0) in game.placement_targets()
    assert game.submit(Move.place(TilePointer(1, 0), PieceKind.Queen))
    assert game.current_player == WHITE

    assert not game.submit(Move.place(TilePointer(10, 10), PieceKind.Queen))
    assert game.current_player == WHITE


def test_invalid_records_raise_value_error() -> None:
    game = hive_native.Game()

    with pytest.raises(ValueError):
        game.submit(cast(Move, (PLACE, 0, 0)))
    with pytest.raises(ValueError):
        game.submit((2, 0, 0, 0, 0, QUEEN))
    with pytest.raises(ValueError):
        game.remaining(2)


def test_board_errors_propagate() -> None:
    game = hive_native.Game()

    with pytest.raises(hive_native.IllegalMoveError):
        game.apply(Move.move(TilePointer(5, 5), ORIGIN, PieceKind.Queen))

    assert issubclass(hive_native.HiveError, RuntimeError)
    assert isinstance(hive_native.IllegalMoveError("x"), hive_native.HiveError)


def test_move_records() -> None:
    placement = Move.place(ORIGIN, PieceKind.Queen)
    assert placement == (PLACE, 0, 0, 0, 0, QUEEN)
    assert placement.is_placement
    assert placement.to_tile == ORIGIN
    assert placement.kind is PieceKind.Queen

    movement = Move.move(ORIGIN, TilePointer(1, 0), PieceKind.Ant)
    assert movement == (MOVE, 0, 0, 1, 0, 4)
    assert not movement.is_placement
    assert movement.from_tile == ORIGIN
    assert movement.to_tile == TilePointer(1, 0)
    assert movement.kind is PieceKind.Ant


def test_board_queries() -> None:
    game = hive_native.Game()
    assert game.submit(Move.place(ORIGIN, PieceKind.Queen))

    assert game.occupied_tiles() == [(0, 0)]
    assert len(game.tiles_around_hive()) == 6
    assert not game.is_empty(0, 0)
    assert game.is_empty(1, 0)
    assert game.remaining(WHITE) == [0, 2, 2, 2, 2]
    assert game.can_place(PieceKind.Ant.value)
    assert game.can_place(PieceKind.Queen.value)


def test_moves_from_converts_to_records() -> None:
    game = hive_native.Game()
    assert game.submit(Move.place(ORIGIN, PieceKind.Queen))
    assert game.submit(Move.place(TilePointer(1, 0), PieceKind.Queen))

    moves = [Move(*record) for record in game.moves_from(0, 0)]
    assert moves
    assert all(not move.is_placement for move in moves)
    assert game.submit(moves[0])
    assert game.current_player == BLACK


def test_enum_parity() -> None:
    assert [kind.value for kind in PieceKind] == list(range(5))
    assert Player.Black.value == BLACK
    assert Player.White.value == WHITE
    assert [result.value for result in GameResult] == list(range(4))
