"""Read-only board queries backed by the native Hive engine."""

from __future__ import annotations

from collections.abc import Iterator
from typing import cast

import hive_native

from hive_client.domain._convert import piece_from_record
from hive_client.domain.errors import EmptyTileError
from hive_client.domain.geometry import TilePointer
from hive_client.domain.pieces import Piece, PieceKind, Player

__all__ = ["BoardView"]


class BoardView:
    """Read-only view over a :class:`hive_native.Game` board."""

    def __init__(self, game: hive_native.Game) -> None:
        """Wrap ``game`` without owning or copying it."""
        self._game = game

    def is_empty_all(self) -> bool:
        """Return True if the board has no pieces at all."""
        return self._game.is_empty_all()

    def is_empty(self, ptr: TilePointer) -> bool:
        """Return True if ``ptr`` has no pieces."""
        return self._game.is_empty(ptr.p, ptr.q)

    def get_top(self, ptr: TilePointer) -> Piece:
        """Return the top piece at ``ptr``, or raise if the tile is empty."""
        top = self._game.top(ptr.p, ptr.q)
        if top is None:
            raise EmptyTileError(f"No pieces at position {ptr}")
        return piece_from_record(cast("tuple[int, int]", top))

    def stacks(self) -> Iterator[tuple[TilePointer, tuple[Piece, ...]]]:
        """Yield ``(tile, pieces)`` snapshots for every occupied tile."""
        for p, q, pieces in self._game.stacks():
            yield TilePointer(p, q), tuple(piece_from_record(piece) for piece in pieces)

    def occupied_tiles(self) -> set[TilePointer]:
        """Return the set of occupied tiles."""
        return {TilePointer(p, q) for p, q in self._game.occupied_tiles()}

    def tiles_around_hive(self) -> set[TilePointer]:
        """Return every empty tile adjacent to an occupied tile."""
        return {TilePointer(p, q) for p, q in self._game.tiles_around_hive()}

    def remaining(self, player: Player) -> dict[PieceKind, int]:
        """Return a copy of ``player``'s remaining piece counts."""
        counts = self._game.remaining(player.value)
        return {kind: counts[kind.value] for kind in PieceKind}

    def has_placed(self, player: Player) -> bool:
        """Return True if ``player`` has placed at least one piece."""
        return self._game.has_placed(player.value)

    def has_placed_queen(self, player: Player) -> bool:
        """Return True if ``player`` has placed their queen."""
        return self._game.has_placed_queen(player.value)
