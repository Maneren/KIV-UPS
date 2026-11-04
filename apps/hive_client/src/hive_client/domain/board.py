"""Mutable Hive board state and rule queries."""

from __future__ import annotations

from collections.abc import Generator, Iterator

from hive_client.domain import movement
from hive_client.domain.errors import EmptyTileError, IllegalMoveError
from hive_client.domain.geometry import (
    Direction,
    TilePointer,
    neighboring_cells,
    rotate_left,
    rotate_right,
)
from hive_client.domain.moves import Move, MovePiece, PlacePiece
from hive_client.domain.pieces import (
    Piece,
    PieceKind,
    Player,
    make_initial_supply,
)

__all__ = ["MAX_STACK_IGNORE", "Board"]

MAX_STACK_IGNORE = 2


class Board:
    """Hive board state: stacks of pieces per tile plus per-player supply."""

    def __init__(self) -> None:
        """Create an empty board with full piece supplies for both players."""
        self._data: dict[TilePointer, list[Piece]] = {}
        self._player_pieces: dict[Player, dict[PieceKind, int]] = {
            Player.White: make_initial_supply(),
            Player.Black: make_initial_supply(),
        }

    # -- access ---------------------------------------------------------

    def get(self, ptr: TilePointer) -> list[Piece]:
        """Return the full (mutable) stack of pieces at ``ptr``."""
        return self._data[ptr]

    def stack_at(self, ptr: TilePointer) -> tuple[Piece, ...]:
        """Return an immutable snapshot of the stack at ``ptr``."""
        return tuple(self._data.get(ptr, ()))

    def stacks(self) -> Iterator[tuple[TilePointer, tuple[Piece, ...]]]:
        """Yield ``(tile, pieces)`` snapshots for every occupied tile."""
        for ptr, stack in self._data.items():
            yield ptr, tuple(stack)

    def occupied_tiles(self) -> set[TilePointer]:
        """Return a copy of the set of occupied tiles."""
        return set(self._data)

    def get_top(
        self,
        ptr: TilePointer,
        ignored: TilePointer | None = None,
    ) -> Piece:
        """Return the top piece at ``ptr``, optionally ignoring the top."""
        stack = self._data.get(ptr)
        if not stack:
            raise EmptyTileError(f"No pieces at position {ptr}")
        if ignored == ptr:
            if len(stack) <= 1:
                raise EmptyTileError(f"No pieces below top at {ptr}")
            return stack[-2]
        return stack[-1]

    def is_empty_all(self) -> bool:
        """Return True if the board has no pieces at all."""
        return not self._data

    def is_empty(
        self,
        ptr: TilePointer,
        ignored: TilePointer | None = None,
    ) -> bool:
        """Return True if ``ptr`` has no (effective) pieces."""
        stack = self._data.get(ptr)
        if not stack:
            return True
        if ignored == ptr:
            return len(stack) <= 1
        return False

    def has_placed(self, player: Player) -> bool:
        """Return True if ``player`` has placed at least one piece."""
        return self._player_pieces[player] != make_initial_supply()

    def has_placed_queen(self, player: Player) -> bool:
        """Return True if ``player`` has placed their queen."""
        return self._player_pieces[player][PieceKind.Queen] == 0

    def pieces(
        self,
        ignored: TilePointer | None = None,
    ) -> Generator[tuple[TilePointer, Piece], None, None]:
        """Yield (ptr, top_piece) for every non-empty tile."""
        for ptr in self._data:
            if self.is_empty(ptr, ignored):
                continue
            yield ptr, self.get_top(ptr, ignored)

    def players_tiles(
        self,
        player: Player,
        ignored: TilePointer | None = None,
    ) -> Generator[tuple[TilePointer, Piece], None, None]:
        """Yield (ptr, piece) for every tile topped by ``player``."""
        for ptr, piece in self.pieces(ignored):
            if piece.owner is player:
                yield ptr, piece

    def remaining(self, player: Player) -> dict[PieceKind, int]:
        """Return a copy of ``player``'s remaining piece counts."""
        return dict(self._player_pieces[player])

    def get_player_pieces(self) -> dict[Player, dict[PieceKind, int]]:
        """Return the per-player remaining piece counts."""
        return self._player_pieces

    # -- mutation -------------------------------------------------------

    def add_piece(self, ptr: TilePointer, piece: Piece) -> None:
        """Push ``piece`` onto the stack at ``ptr``."""
        self._data.setdefault(ptr, []).append(piece)

    def remove_piece(self, ptr: TilePointer) -> Piece:
        """Pop and return the top piece at ``ptr``."""
        stack = self._data.get(ptr)
        if not stack:
            raise EmptyTileError("Tried to remove piece from empty tile")
        piece = stack.pop()
        if not stack:
            del self._data[ptr]
        return piece

    def apply_move(self, move: Move, player: Player) -> None:
        """Apply ``move`` on behalf of ``player``, mutating the board."""
        if isinstance(move, MovePiece):
            stack = self._data.get(move.from_)
            if not stack:
                raise EmptyTileError(f"No piece to move at {move.from_}")
            if stack[-1].kind != move.piece_kind:
                raise IllegalMoveError("Tried to move piece of different kind")
            self.add_piece(move.to, self.remove_piece(move.from_))
        elif isinstance(move, PlacePiece):
            pos, kind = move.pos, move.piece_kind
            available = self._player_pieces[player][kind]
            if available == 0:
                raise IllegalMoveError("Attempted to add piece when no pieces left")
            self.add_piece(pos, Piece(kind=kind, owner=player))
            self._player_pieces[player][kind] -= 1
        else:  # pragma: no cover - unreachable for a closed union
            raise TypeError(f"Unknown move type: {type(move)!r}")

    def clone(self) -> Board:
        """Return a deep copy of this board."""
        clone = Board()
        clone._data = {ptr: list(stack) for ptr, stack in self._data.items()}
        clone._player_pieces = {
            player: dict(kinds) for player, kinds in self._player_pieces.items()
        }
        return clone

    # -- rules ----------------------------------------------------------

    def can_player_place(self, player: Player, kind: PieceKind) -> bool:
        """Return True if ``player`` still has a ``kind`` piece to place."""
        return self._player_pieces[player][kind] > 0

    def can_player_place_at(self, player: Player, ptr: TilePointer) -> bool:
        """Return True if ``player`` may place at ``ptr``."""
        return self.is_empty(ptr) and (
            not self.has_placed(player) or self.neighbors_only_players(ptr, player)
        )

    def tiles_around_hive(
        self,
        ignored: TilePointer | None = None,
    ) -> set[TilePointer]:
        """Return all empty tiles adjacent to any occupied tile."""
        tiles: set[TilePointer] = set()
        for ptr in self._data:
            if self.is_empty(ptr, ignored):
                continue
            tiles.update(self.empty_neighbors(ptr, ignored))
        return tiles

    def valid_placements(
        self,
        player: Player,
    ) -> Generator[TilePointer, None, None]:
        """Yield tiles where ``player`` may place a piece."""
        for ptr in self.tiles_around_hive():
            if self.neighbors_only_players(ptr, player):
                yield ptr

    def moves_for_piece(
        self,
        pos: TilePointer,
        piece: Piece,
    ) -> Generator[Move, None, None]:
        """Yield all legal moves for the piece at ``pos``."""
        yield from movement.moves_for_piece(self, pos, piece.kind)

    def moves_for_player(
        self,
        player: Player,
        pieces: dict[PieceKind, int],
    ) -> Generator[Move, None, None]:
        """Yield all legal moves for ``player`` given remaining pieces."""
        for placement_ptr in self.valid_placements(player):
            for piece_kind, count in pieces.items():
                if count == 0:
                    continue
                yield PlacePiece(pos=placement_ptr, piece_kind=piece_kind)

        if self.has_placed_queen(player):
            for pos, piece in self.moveable_pieces_for(player):
                yield from self.moves_for_piece(pos, piece)

    def neighbors(
        self,
        ptr: TilePointer,
        ignored: TilePointer | None = None,
    ) -> Generator[TilePointer, None, None]:
        """Yield occupied neighboring cells of ``ptr``."""
        for neighbor in neighboring_cells(ptr):
            if not self.is_empty(neighbor, ignored):
                yield neighbor

    def empty_neighbors(
        self,
        ptr: TilePointer,
        ignored: TilePointer | None = None,
    ) -> Generator[TilePointer, None, None]:
        """Yield empty neighboring cells of ``ptr``."""
        for neighbor in neighboring_cells(ptr):
            if self.is_empty(neighbor, ignored):
                yield neighbor

    def tile_belongs_to_player(
        self,
        ptr: TilePointer,
        player: Player,
        ignored: TilePointer | None = None,
    ) -> bool:
        """Return True if the top piece at ``ptr`` belongs to ``player``."""
        return self.get_top(ptr, ignored).owner is player

    def neighbors_only_players(
        self,
        ptr: TilePointer,
        player: Player,
        ignored: TilePointer | None = None,
    ) -> bool:
        """Return True if every neighbor of ``ptr`` belongs to ``player``."""
        return all(
            self.tile_belongs_to_player(n, player, ignored)
            for n in self.neighbors(ptr, ignored)
        )

    def can_player_move(self, player: Player, ptr: TilePointer) -> bool:
        """Return True if ``player`` may move the piece at ``ptr``."""
        return (
            not self.is_empty(ptr)
            and self.has_placed_queen(player)
            and self.get_top(ptr).owner is player
            and not self.moving_breaks_hive(ptr)
        )

    def moving_breaks_hive(self, ptr: TilePointer) -> bool:
        """Return True if lifting the piece at ``ptr`` splits the hive."""
        if len(self._data) <= MAX_STACK_IGNORE:
            return False

        stack = self._data.get(ptr)
        if not stack:
            raise EmptyTileError(f"No piece to lift at {ptr}")
        if len(stack) > 1:
            return False

        # Seed the BFS from a single neighbor only. If we seeded from all
        # neighbors simultaneously, pieces adjacent to ``ptr`` would always be
        # marked reachable even when they're only connected through ``ptr``.
        first_neighbor = next(self.neighbors(ptr, ptr), None)
        assert first_neighbor is not None, (
            "There should be at least one neighbor, unless the hive is already split."
        )

        visited: set[TilePointer] = set()
        stack_to_visit = [first_neighbor]

        while stack_to_visit:
            current = stack_to_visit.pop()
            if current in visited:
                continue
            visited.add(current)
            stack_to_visit.extend(self.neighbors(current, ptr))

        return any(p not in visited for p, _ in self.pieces(ptr))

    def moveable_pieces_for(
        self,
        player: Player,
    ) -> Generator[tuple[TilePointer, Piece], None, None]:
        """Yield (ptr, piece) for every movable piece of ``player``."""
        for pos, piece in self.players_tiles(player):
            if not self.can_player_move(player, pos):
                continue
            yield pos, piece

    def can_move_to(
        self,
        from_: TilePointer,
        to: TilePointer,
        *,
        can_leave: bool = False,
        ignored: TilePointer | None = None,
    ) -> bool:
        """Return True if a piece may step from ``from_`` to ``to``."""
        direction = to - from_

        left = from_ + rotate_left(direction)
        right = from_ + rotate_right(direction)

        left_empty = self.is_empty(left, ignored)
        right_empty = self.is_empty(right, ignored)

        has_gap = left_empty != right_empty
        both_empty_and_can_leave = left_empty and right_empty and can_leave
        has_neighbor_in_to_dir = self.has_neighbor_in_direction(to, direction, ignored)

        return has_gap or (both_empty_and_can_leave and has_neighbor_in_to_dir)

    def valid_steps(
        self,
        ptr: TilePointer,
        ignored: TilePointer | None = None,
        *,
        can_leave: bool = False,
    ) -> Generator[TilePointer, None, None]:
        """Yield empty neighbors of ``ptr`` that can be moved to."""
        for neighbor in self.empty_neighbors(ptr, ignored):
            if self.can_move_to(ptr, neighbor, can_leave=can_leave, ignored=ignored):
                yield neighbor

    def has_neighbor_in_direction(
        self,
        cell: TilePointer,
        direction: Direction,
        ignored: TilePointer | None = None,
    ) -> bool:
        """Return True if any cell in the ``direction`` arc of ``cell`` is occupied."""
        center = cell + direction
        left = cell + rotate_left(direction)
        right = cell + rotate_right(direction)

        return (
            not self.is_empty(left, ignored)
            or not self.is_empty(right, ignored)
            or not self.is_empty(center, ignored)
        )

    def has_neighbor(
        self,
        cell: TilePointer,
        ignored: TilePointer | None = None,
    ) -> bool:
        """Return True if ``cell`` has at least one occupied neighbor."""
        if len(self._data) == MAX_STACK_IGNORE:
            return True
        return any(not self.is_empty(ptr, ignored) for ptr in neighboring_cells(cell))

    def surrounding_count(self, ptr: TilePointer) -> int:
        """Return how many of the six neighbours of ``ptr`` are occupied."""
        return sum(1 for _ in self.neighbors(ptr))

    # -- per-piece move generators -------------------------------------

    def grasshopper_moves(
        self,
        grasshopper: TilePointer,
    ) -> Generator[Move, None, None]:
        """Yield grasshopper moves: jump over a line of pieces."""
        yield from movement.grasshopper_moves(self, grasshopper)

    def queens_moves(self, queen: TilePointer) -> Generator[Move, None, None]:
        """Yield queen moves: one step to any adjacent empty cell."""
        yield from movement.queen_moves(self, queen)

    def beetle_moves(self, beetle: TilePointer) -> Generator[Move, None, None]:
        """Yield beetle moves: onto any neighboring occupied cell."""
        yield from movement.beetle_moves(self, beetle)

    def spider_moves(self, spider: TilePointer) -> Generator[Move, None, None]:
        """Yield spider moves: exactly 3 steps without backtracking."""
        yield from movement.spider_moves(self, spider)

    def ant_moves(self, ant: TilePointer) -> Generator[Move, None, None]:
        """Yield ant moves: any number of steps around the hive."""
        yield from movement.ant_moves(self, ant)
