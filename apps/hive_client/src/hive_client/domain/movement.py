"""
Per-piece movement rules.

The functions here are pure with respect to board mutation: they only *read* a
:class:`~hive_client.domain.board.Board` and yield legal moves.  Keeping them
outside ``Board`` separates the mutable game state from the movement
algorithms and makes each rule independently testable.
"""

from __future__ import annotations

from collections import deque
from collections.abc import Callable, Iterator
from typing import TYPE_CHECKING

from hive_client.domain.geometry import (
    DIRECTIONS,
    TilePointer,
    neighboring_cells,
)
from hive_client.domain.moves import Move, MovePiece
from hive_client.domain.pieces import PieceKind

if TYPE_CHECKING:
    from hive_client.domain.board import Board

__all__ = [
    "MOVE_GENERATORS",
    "MoveGenerator",
    "ant_moves",
    "beetle_moves",
    "grasshopper_moves",
    "moves_for_piece",
    "queen_moves",
    "spider_moves",
]

MoveGenerator = Callable[["Board", TilePointer], Iterator[Move]]


def _move(from_: TilePointer, to: TilePointer, kind: PieceKind) -> Move:
    return MovePiece(from_=from_, to=to, piece_kind=kind)


def moves_for_piece(
    board: Board,
    pos: TilePointer,
    kind: PieceKind,
) -> Iterator[Move]:
    """Yield every legal move for a ``kind`` piece standing on ``pos``."""
    yield from MOVE_GENERATORS[kind](board, pos)


def grasshopper_moves(board: Board, grasshopper: TilePointer) -> Iterator[Move]:
    """Yield grasshopper moves: jump over a line of pieces."""
    for direction in DIRECTIONS:
        current = grasshopper
        skipped = False
        while True:
            current = current + direction
            if board.is_empty(current, grasshopper):
                if skipped:
                    yield _move(grasshopper, current, PieceKind.Grasshopper)
                break
            skipped = True


def queen_moves(board: Board, queen: TilePointer) -> Iterator[Move]:
    """Yield queen moves: one step to any adjacent empty cell."""
    for step_ptr in board.valid_steps(queen, queen, can_leave=True):
        yield _move(queen, step_ptr, PieceKind.Queen)


def beetle_moves(board: Board, beetle: TilePointer) -> Iterator[Move]:
    """Yield beetle moves: onto any neighboring occupied cell."""
    for ptr in neighboring_cells(beetle):
        if board.has_neighbor(ptr, beetle):
            yield _move(beetle, ptr, PieceKind.Beetle)


def spider_moves(board: Board, spider: TilePointer) -> Iterator[Move]:
    """Yield spider moves: exactly 3 steps without backtracking."""
    visited: set[TilePointer] = set()
    stack: list[TilePointer] = [spider]

    for _ in range(3):
        new_stack: list[TilePointer] = []
        for ptr in stack:
            visited.add(ptr)
            new_stack.extend(
                step_ptr
                for step_ptr in board.valid_steps(ptr, spider)
                if step_ptr not in visited
            )
        if not new_stack:
            break
        stack = new_stack

    for ptr in stack:
        yield _move(spider, ptr, PieceKind.Spider)


def ant_moves(board: Board, ant: TilePointer) -> Iterator[Move]:
    """Yield ant moves: any number of steps around the hive."""
    visited: set[TilePointer] = {ant}
    queue: deque[TilePointer] = deque(board.valid_steps(ant, ant))

    while queue:
        current = queue.popleft()
        if current in visited:
            continue
        visited.add(current)
        yield _move(ant, current, PieceKind.Ant)
        queue.extend(board.valid_steps(current, ant))


MOVE_GENERATORS: dict[PieceKind, MoveGenerator] = {
    PieceKind.Queen: queen_moves,
    PieceKind.Spider: spider_moves,
    PieceKind.Beetle: beetle_moves,
    PieceKind.Grasshopper: grasshopper_moves,
    PieceKind.Ant: ant_moves,
}
