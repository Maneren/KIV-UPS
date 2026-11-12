"""Differential test between the legacy Python rules and the native engine."""

from __future__ import annotations

import random

import hive_native
from hive_client.domain import (
    GameSession,
    Move,
    MovePiece,
    PieceKind,
    PlacePiece,
    Player,
    TilePointer,
    make_placement,
)
from hypothesis import given, settings
from hypothesis import strategies as st

PLACE = 0
MOVE = 1
PIECE_KINDS = range(5)


def to_record(move: Move) -> tuple[int, int, int, int, int, int]:
    if isinstance(move, PlacePiece):
        return (
            PLACE,
            0,
            0,
            move.pos.p,
            move.pos.q,
            move.piece_kind.value,
        )
    return (
        MOVE,
        move.from_.p,
        move.from_.q,
        move.to.p,
        move.to.q,
        move.piece_kind.value,
    )


def from_record(
    record: tuple[int, int, int, int, int, int],
) -> Move:
    kind = PieceKind(record[5])
    if record[0] == PLACE:
        return PlacePiece(pos=TilePointer(record[3], record[4]), piece_kind=kind)
    return MovePiece(
        from_=TilePointer(record[1], record[2]),
        to=TilePointer(record[3], record[4]),
        piece_kind=kind,
    )


def start_engines() -> tuple[GameSession, hive_native.Game]:
    reference = GameSession()
    native = hive_native.Game()

    opening = [
        make_placement(TilePointer(0, 0), PieceKind.Queen),
        make_placement(TilePointer(1, 0), PieceKind.Queen),
    ]
    for move in opening:
        reference.apply(move)
        native.apply(to_record(move))

    return reference, native


def compare_state(reference: GameSession, native: hive_native.Game) -> None:
    assert reference.current_player.value == native.current_player
    assert reference.result.value == native.result
    assert reference.winner is None or reference.winner.value == native.winner

    reference_stacks = sorted(
        (
            (ptr.p, ptr.q, [(piece.kind.value, piece.owner.value) for piece in stack])
            for ptr, stack in reference.board.stacks()
        ),
        key=lambda stack: (stack[0], stack[1]),
    )
    assert reference_stacks == native.stacks()

    for player in (Player.Black, Player.White):
        remaining = reference.board.remaining(player)
        expected = [remaining[kind] for kind in map(PieceKind, PIECE_KINDS)]
        assert expected == native.remaining(player.value)

    reference_targets = sorted((ptr.p, ptr.q) for ptr in reference.placement_targets())
    assert reference_targets == native.placement_targets()

    reference_moves = sorted(to_record(move) for move in reference.legal_moves())
    assert reference_moves == sorted(native.legal_moves())


@settings(max_examples=20, deadline=None)
@given(seed=st.integers(min_value=0, max_value=2**32 - 1))
def test_native_matches_reference(seed: int) -> None:
    rng = random.Random(seed)
    reference, native = start_engines()

    for _ in range(80):
        compare_state(reference, native)
        if reference.is_over():
            return

        moves = sorted(reference.legal_moves(), key=to_record)
        if not moves:
            reference_passed = reference.pass_if_stuck()
            native_passed = native.pass_if_stuck()
            assert reference_passed == native_passed
            if not reference_passed:
                return
            continue

        move = rng.choice(moves)
        record = to_record(move)
        reference_accepted = reference.submit(move)
        native_accepted = native.submit(record)
        assert reference_accepted == native_accepted

        if not reference_accepted:
            return


def test_illegal_moves_agree() -> None:
    reference, native = start_engines()

    illegal_records = [
        (MOVE, 9, 9, 0, 0, PieceKind.Queen.value),
        (MOVE, 0, 0, 0, -1, PieceKind.Ant.value),
        (PLACE, 0, 0, 0, -1, PieceKind.Queen.value),
    ]

    for record in illegal_records:
        assert not reference.submit(from_record(record))
        assert not native.submit(record)
        compare_state(reference, native)
