"""
Hive game engine.

Pure Python port of the C++ `hive` library rules. Hex grid uses axial
coordinates (p, q).
"""

from __future__ import annotations

from collections.abc import Generator, Iterator
from dataclasses import dataclass
from enum import Enum
from typing import overload

__all__ = [
    "DEFAULT_PIECES_PER_PLAYER",
    "NUMBER_OF_PIECES",
    "Board",
    "Move",
    "MovePiece",
    "Piece",
    "PieceKind",
    "PlacePiece",
    "Player",
    "TilePointer",
    "make_move",
    "make_placement",
]


@dataclass(frozen=True)
class Direction:
    p: int
    q: int


DIRECTIONS: tuple[Direction, ...] = (
    Direction(1, 0),
    Direction(0, 1),
    Direction(-1, 1),
    Direction(-1, 0),
    Direction(0, -1),
    Direction(1, -1),
)

MAX_STACK_IGNORE = 2
HEX_SIDES = 6


class Player(Enum):
    """Player colour: White moves first, Black second."""

    Black = 0
    White = 1

    def other(self) -> Player:
        """Return the opposing player."""
        return Player.Black if self is Player.White else Player.White


class PieceKind(Enum):
    """Kind of Hive piece."""

    Queen = 0
    Spider = 1
    Beetle = 2
    Grasshopper = 3
    Ant = 4


NUMBER_OF_PIECES = 5

DEFAULT_PIECES_PER_PLAYER = {
    PieceKind.Queen: 1,
    PieceKind.Spider: 2,
    PieceKind.Beetle: 2,
    PieceKind.Grasshopper: 2,
    PieceKind.Ant: 2,
}


@dataclass(frozen=True)
class Piece:
    """A piece on the board: kind plus owning player."""

    kind: PieceKind
    owner: Player

    def display(self) -> str:
        """Return the uppercase kind for White, lowercase for Black."""
        name = self.kind.name
        if self.owner is Player.White:
            return name.upper()
        return name.lower()


@dataclass(frozen=True)
class TilePointer:
    """Axial hex coordinate (p, q)."""

    p: int
    q: int

    def __hash__(self) -> int:
        """Hash by XOR of coordinates."""
        return hash(self.p) ^ hash(self.q)

    def __str__(self) -> str:
        """Return coordinate as ``(p, q)``."""
        return f"({self.p}, {self.q})"

    def __add__(self, other: Direction) -> TilePointer:
        """Add a direction to a tile pointer."""
        return TilePointer(self.p + other.p, self.q + other.q)

    @overload
    def __sub__(self, other: Direction) -> TilePointer: ...

    @overload
    def __sub__(self, other: TilePointer) -> Direction: ...

    def __sub__(self, other: Direction | TilePointer) -> Direction | TilePointer:
        """Subtract a direction or another tile from a tile pointer."""
        return TilePointer(self.p - other.p, self.q - other.q)


@dataclass(frozen=True)
class PlacePiece:
    """Place a piece of ``piece_kind`` at ``pos``."""

    pos: TilePointer
    piece_kind: PieceKind


@dataclass(frozen=True)
class MovePiece:
    """Move a piece of ``piece_kind`` from ``from_`` to ``to``."""

    from_: TilePointer
    to: TilePointer
    piece_kind: PieceKind


Move = PlacePiece | MovePiece


def make_move(from_: TilePointer, to: TilePointer, piece_kind: PieceKind) -> Move:
    """Create a ``MovePiece`` move."""
    return MovePiece(from_=from_, to=to, piece_kind=piece_kind)


def make_placement(pos: TilePointer, piece_kind: PieceKind) -> Move:
    """Create a ``PlacePiece`` move."""
    return PlacePiece(pos=pos, piece_kind=piece_kind)


def rotate_left(direction: Direction) -> Direction:
    """Rotate a direction 60 degrees counter-clockwise."""
    return Direction(direction.p + direction.q, -direction.p)


def rotate_right(direction: Direction) -> Direction:
    """Rotate a direction 60 degrees clockwise."""
    return Direction(-direction.q, direction.p + direction.q)


def neighboring_cells(ptr: TilePointer) -> Iterator[TilePointer]:
    """Yield the six neighboring hex cells of ``ptr``."""
    for direction in DIRECTIONS:
        yield ptr + direction


class Board:
    """Hive board state: stacks of pieces per tile plus per-player supply."""

    def __init__(self) -> None:
        """Create an empty board with full piece supplies for both players."""
        self._data: dict[TilePointer, list[Piece]] = {}
        self._player_pieces: dict[Player, dict[PieceKind, int]] = {
            Player.White: dict(DEFAULT_PIECES_PER_PLAYER),
            Player.Black: dict(DEFAULT_PIECES_PER_PLAYER),
        }

    # -- access ---------------------------------------------------------

    def get(self, ptr: TilePointer) -> list[Piece]:
        """Return the full stack of pieces at ``ptr``."""
        return self._data[ptr]

    def get_top(
        self,
        ptr: TilePointer,
        ignored: TilePointer | None = None,
    ) -> Piece:
        """Return the top piece at ``ptr``, optionally ignoring the top."""
        stack = self._data[ptr]
        if ignored == ptr:
            if len(stack) <= 1:
                raise RuntimeError("No pieces at position")
            return stack[-2]
        if not stack:
            raise RuntimeError("No pieces at position")
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
        return self._player_pieces[player] != dict(DEFAULT_PIECES_PER_PLAYER)

    def has_placed_queen(self, player: Player) -> bool:
        """Return True if ``player`` has placed their queen."""
        return self._player_pieces[player][PieceKind.Queen] == 0

    def pieces(
        self, ignored: TilePointer | None = None
    ) -> Generator[tuple[TilePointer, Piece], None, None]:
        """Yield (ptr, top_piece) for every non-empty tile."""
        for ptr in self._data:
            if self.is_empty(ptr, ignored):
                continue
            yield ptr, self.get_top(ptr, ignored)

    def players_tiles(
        self, player: Player, ignored: TilePointer | None = None
    ) -> Generator[tuple[TilePointer, Piece], None, None]:
        """Yield (ptr, piece) for every tile topped by ``player``."""
        for ptr, piece in self.pieces(ignored):
            if piece.owner is player:
                yield ptr, piece

    # -- mutation -------------------------------------------------------

    def add_piece(self, ptr: TilePointer, piece: Piece) -> None:
        """Push ``piece`` onto the stack at ``ptr``."""
        self._data.setdefault(ptr, []).append(piece)

    def remove_piece(self, ptr: TilePointer) -> Piece:
        """Pop and return the top piece at ``ptr``."""
        stack = self._data.get(ptr)
        if not stack:
            raise RuntimeError("Tried to remove piece from empty tile")
        piece = stack.pop()
        if not stack:
            del self._data[ptr]
        return piece

    def apply_move(self, move: Move, player: Player) -> None:
        """Apply ``move`` on behalf of ``player``, mutating the board."""
        if isinstance(move, MovePiece):
            piece = self.remove_piece(move.from_)
            if piece.kind != move.piece_kind:
                raise RuntimeError("Tried to move piece of different kind")
            self.add_piece(move.to, piece)
        elif isinstance(move, PlacePiece):
            pos, kind = move.pos, move.piece_kind
            available = self._player_pieces[player][kind]
            if available == 0:
                raise RuntimeError("Attempted to add piece when no pieces left")
            self.add_piece(pos, Piece(kind=kind, owner=player))
            self._player_pieces[player][kind] -= 1
        else:  # pragma: no cover - unreachable for a closed union
            raise TypeError(f"Unknown move type: {type(move)!r}")

    def get_player_pieces(self) -> dict[Player, dict[PieceKind, int]]:
        """Return the per-player remaining piece counts."""
        return self._player_pieces

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

    def tiles_around_hive(self, ignored: TilePointer | None = None) -> set[TilePointer]:
        """Return all empty tiles adjacent to any occupied tile."""
        tiles: set[TilePointer] = set()
        for ptr in self._data:
            if self.is_empty(ptr, ignored):
                continue
            for neighbor in self.empty_neighbors(ptr, ignored):
                tiles.add(neighbor)
        return tiles

    def valid_placements(self, player: Player) -> Generator[TilePointer, None, None]:
        """Yield tiles where ``player`` may place a piece."""
        for ptr in self.tiles_around_hive():
            if self.neighbors_only_players(ptr, player):
                yield ptr

    def moves_for_piece(
        self, pos: TilePointer, piece: Piece
    ) -> Generator[Move, None, None]:
        """Yield all legal moves for the piece at ``pos``."""
        handler = _PIECE_HANDLERS[piece.kind.value]
        yield from handler(self, pos)

    def moves_for_player(
        self, player: Player, pieces: dict[PieceKind, int]
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
        self, ptr: TilePointer, ignored: TilePointer | None = None
    ) -> Generator[TilePointer, None, None]:
        """Yield occupied neighboring cells of ``ptr``."""
        for neighbor in neighboring_cells(ptr):
            if not self.is_empty(neighbor, ignored):
                yield neighbor

    def empty_neighbors(
        self, ptr: TilePointer, ignored: TilePointer | None = None
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

        tile = self._data[ptr]
        if len(tile) > 1:
            return False

        # Seed the BFS from a single neighbor only. If we seeded from all
        # neighbors simultaneously, pieces adjacent to ``ptr`` would always be
        # marked reachable even when they're only connected through ``ptr``.
        first_neighbor = next(self.neighbors(ptr, ptr), None)
        assert first_neighbor is not None, (
            "There should be at least one neighbor, unless the hive is already split."
        )

        visited: set[TilePointer] = set()
        stack = [first_neighbor]

        while stack:
            current = stack.pop()
            if current in visited:
                continue
            visited.add(current)
            stack.extend(self.neighbors(current, ptr))

        return any(p not in visited for p, _ in self.pieces(ptr))

    def moveable_pieces_for(
        self, player: Player
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
        self, cell: TilePointer, ignored: TilePointer | None = None
    ) -> bool:
        """Return True if ``cell`` has at least one occupied neighbor."""
        if len(self._data) == MAX_STACK_IGNORE:
            return True
        return any(not self.is_empty(ptr, ignored) for ptr in neighboring_cells(cell))

    # -- per-piece move generators -------------------------------------

    def grasshopper_moves(
        self, grasshopper: TilePointer
    ) -> Generator[Move, None, None]:
        """Yield grasshopper moves: jump over a line of pieces."""
        for direction in DIRECTIONS:
            current = grasshopper
            skipped = False
            while True:
                current = current + direction
                if self.is_empty(current, grasshopper):
                    if skipped:
                        yield MovePiece(
                            from_=grasshopper,
                            to=current,
                            piece_kind=PieceKind.Grasshopper,
                        )
                    break
                skipped = True

    def queens_moves(self, queen: TilePointer) -> Generator[Move, None, None]:
        """Yield queen moves: one step to any adjacent empty cell."""
        for step_ptr in self.valid_steps(queen, queen, can_leave=True):
            yield MovePiece(from_=queen, to=step_ptr, piece_kind=PieceKind.Queen)

    def beetle_moves(self, beetle: TilePointer) -> Generator[Move, None, None]:
        """Yield beetle moves: onto any neighboring occupied cell."""
        for ptr in neighboring_cells(beetle):
            if self.has_neighbor(ptr, beetle):
                yield MovePiece(from_=beetle, to=ptr, piece_kind=PieceKind.Beetle)

    def spider_moves(self, spider: TilePointer) -> Generator[Move, None, None]:
        """Yield spider moves: exactly 3 steps without backtracking."""
        visited: set[TilePointer] = set()
        stack: list[TilePointer] = [spider]

        for _ in range(3):
            new_stack: list[TilePointer] = []
            for ptr in stack:
                visited.add(ptr)
                new_stack.extend(
                    step_ptr
                    for step_ptr in self.valid_steps(ptr, spider)
                    if step_ptr not in visited
                )
            if not new_stack:
                break
            stack = new_stack.copy()

        for ptr in stack:
            yield MovePiece(from_=spider, to=ptr, piece_kind=PieceKind.Spider)

    def ant_moves(self, ant: TilePointer) -> Generator[Move, None, None]:
        """Yield ant moves: any number of steps around the hive."""
        visited: set[TilePointer] = {ant}
        queue: list[TilePointer] = []

        queue.extend(self.valid_steps(ant, ant))

        while queue:
            current = queue.pop(0)
            if current in visited:
                continue
            visited.add(current)
            yield MovePiece(from_=ant, to=current, piece_kind=PieceKind.Ant)
            queue.extend(self.valid_steps(current, ant))


_PIECE_HANDLERS = (
    Board.queens_moves,
    Board.spider_moves,
    Board.beetle_moves,
    Board.grasshopper_moves,
    Board.ant_moves,
)


class GameResult(Enum):
    """Outcome of a Hive game."""

    ONGOING = 0
    WHITE_WINS = 1
    BLACK_WINS = 2
    DRAW = 3


class GameState:
    """Hive game state: board, current player, and win/draw detection."""

    def __init__(self) -> None:
        self.board = Board()
        self.current_player = Player.White
        self.result = GameResult.ONGOING

    def legal_moves(self) -> list[Move]:
        """Return all legal moves for the current player."""
        return list(
            self.board.moves_for_player(
                self.current_player, self.board.get_player_pieces()[self.current_player]
            )
        )

    def apply(self, move: Move) -> None:
        """Apply ``move`` on behalf of the current player, then switch turns."""
        self.board.apply_move(move, self.current_player)
        self.current_player = self.current_player.other()
        self._check_win()

    def _check_win(self) -> None:
        """Set ``result`` if either queen is surrounded."""
        for player in (Player.White, Player.Black):
            queen_pos = self._find_queen(player)
            if queen_pos is None:
                continue
            occupied_neighbors = sum(
                not self.board.is_empty(n) for n in self.board.neighbors(queen_pos)
            )
            if occupied_neighbors == HEX_SIDES:
                self.result = (
                    GameResult.BLACK_WINS
                    if player is Player.White
                    else GameResult.WHITE_WINS
                )
                return

    def _find_queen(self, player: Player) -> TilePointer | None:
        """Return the tile of ``player``'s queen, or None if not placed."""
        for ptr, piece in self.board.pieces():
            if piece.owner is player and piece.kind is PieceKind.Queen:
                return ptr
        return None
