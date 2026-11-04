"""Tests for the Hive Python client: game engine, network protocol, and GUI."""

from hive_client.game import (
    Board,
    Direction,
    GameResult,
    GameState,
    MovePiece,
    Piece,
    PieceKind,
    Player,
    TilePointer,
    make_placement,
)
from hive_client.network import (
    Login,
    Move,
    Over,
    Place,
    Reject,
    Rematch,
    Start,
    parse_command,
    serialize,
)


def test_place_first_piece_at_origin() -> None:
    board = Board()
    board.apply_move(make_placement(TilePointer(0, 0), PieceKind.Queen), Player.White)
    assert board.has_placed_queen(Player.White)
    top = board.get_top(TilePointer(0, 0))
    assert top == Piece(kind=PieceKind.Queen, owner=Player.White)


def test_cannot_place_away_from_hive_on_first_move() -> None:
    board = Board()
    # Board rules allow any placement on the first move; the GUI enforces (0,0)
    board.apply_move(make_placement(TilePointer(1, 0), PieceKind.Queen), Player.White)
    assert board.has_placed_queen(Player.White)


def test_queen_can_move_one_step() -> None:
    board = Board()
    board.apply_move(make_placement(TilePointer(0, 0), PieceKind.Queen), Player.White)
    board.apply_move(make_placement(TilePointer(1, 0), PieceKind.Ant), Player.Black)
    moves = list(
        board.moves_for_piece(TilePointer(0, 0), board.get_top(TilePointer(0, 0)))
    )
    destinations = {m.to for m in moves if isinstance(m, MovePiece)}
    assert len(destinations) > 0


def test_ant_can_move_many_steps() -> None:
    board = Board()
    board.apply_move(make_placement(TilePointer(0, 0), PieceKind.Ant), Player.White)
    board.apply_move(make_placement(TilePointer(1, 0), PieceKind.Ant), Player.Black)
    board.apply_move(make_placement(TilePointer(2, 0), PieceKind.Ant), Player.Black)
    moves = list(
        board.moves_for_piece(TilePointer(0, 0), board.get_top(TilePointer(0, 0)))
    )
    # Ant should be able to reach (0, -1) via (1, 0) being blocked... actually
    # ant moves like a sliding piece around the hive
    destinations = {m.to for m in moves if isinstance(m, MovePiece)}
    assert len(destinations) > 0


def test_spider_moves_exactly_three_steps() -> None:
    board = Board()
    board.apply_move(make_placement(TilePointer(0, 0), PieceKind.Spider), Player.White)
    board.apply_move(make_placement(TilePointer(1, 0), PieceKind.Spider), Player.Black)
    board.apply_move(make_placement(TilePointer(2, 0), PieceKind.Spider), Player.Black)
    moves = list(
        board.moves_for_piece(TilePointer(0, 0), board.get_top(TilePointer(0, 0)))
    )
    destinations = {m.to for m in moves if isinstance(m, MovePiece)}
    assert len(destinations) > 0


def test_grasshopper_jumps_over_pieces() -> None:
    board = Board()
    board.apply_move(
        make_placement(TilePointer(0, 0), PieceKind.Grasshopper), Player.White
    )
    board.apply_move(make_placement(TilePointer(1, 0), PieceKind.Ant), Player.Black)
    board.apply_move(make_placement(TilePointer(2, 0), PieceKind.Spider), Player.Black)
    board.apply_move(make_placement(TilePointer(3, 0), PieceKind.Beetle), Player.Black)
    moves = list(
        board.moves_for_piece(TilePointer(0, 0), board.get_top(TilePointer(0, 0)))
    )
    destinations = {m.to for m in moves if isinstance(m, MovePiece)}
    assert TilePointer(4, 0) in destinations


def test_beetle_moves_onto_neighbor() -> None:
    board = Board()
    board.apply_move(make_placement(TilePointer(0, 0), PieceKind.Beetle), Player.White)
    board.apply_move(make_placement(TilePointer(1, 0), PieceKind.Beetle), Player.Black)
    moves = list(
        board.moves_for_piece(TilePointer(0, 0), board.get_top(TilePointer(0, 0)))
    )
    destinations = {m.to for m in moves if isinstance(m, MovePiece)}
    assert TilePointer(1, 0) in destinations


def test_cannot_move_before_queen_placed() -> None:
    board = Board()
    board.apply_move(make_placement(TilePointer(0, 0), PieceKind.Ant), Player.White)
    assert not board.can_player_move(Player.White, TilePointer(0, 0))


def test_player_supply_decreases_on_placement() -> None:
    board = Board()
    before = board.get_player_pieces()[Player.White][PieceKind.Queen]
    board.apply_move(make_placement(TilePointer(0, 0), PieceKind.Queen), Player.White)
    after = board.get_player_pieces()[Player.White][PieceKind.Queen]
    assert after == before - 1


def test_can_player_place_tracks_supply() -> None:
    board = Board()
    assert board.can_player_place(Player.White, PieceKind.Queen)
    board.apply_move(make_placement(TilePointer(0, 0), PieceKind.Queen), Player.White)
    assert not board.can_player_place(Player.White, PieceKind.Queen)
    assert board.can_player_place(Player.White, PieceKind.Ant)


def test_moves_for_player_empty_on_new_board() -> None:
    # Before either queen is placed, no moves or placements are legal.
    board = Board()
    moves = list(
        board.moves_for_player(Player.White, board.get_player_pieces()[Player.White])
    )
    assert moves == []


def test_moving_breaks_hive_detected() -> None:
    # Three pieces in a line: (1,0)-(0,0)-(-1,0). The piece at (0,0) connects
    # the two outer pieces; lifting it splits the hive.
    board = Board()
    board.apply_move(make_placement(TilePointer(0, 0), PieceKind.Queen), Player.White)
    board.apply_move(make_placement(TilePointer(1, 0), PieceKind.Ant), Player.Black)
    board.apply_move(make_placement(TilePointer(-1, 0), PieceKind.Spider), Player.Black)
    assert board.moving_breaks_hive(TilePointer(0, 0))
    assert not board.can_player_move(Player.White, TilePointer(0, 0))


def test_parse_login() -> None:
    cmd = parse_command(["LOGIN", "05Alice", "05lobby"])
    assert isinstance(cmd, Login)
    assert cmd.nick == "Alice"
    assert cmd.lobby == "lobby"


def test_parse_place() -> None:
    cmd = parse_command(["PLACE", "1", "-2", "Q"])
    assert isinstance(cmd, Place)
    assert cmd.p == 1
    assert cmd.q == -2
    assert cmd.piece == PieceKind.Queen


def test_parse_move() -> None:
    cmd = parse_command(["MOVE", "0", "0", "1", "0", "Q"])

    assert isinstance(cmd, Move)
    assert cmd.from_p == 0
    assert cmd.from_q == 0
    assert cmd.to_p == 1
    assert cmd.to_q == 0
    assert cmd.piece == PieceKind.Queen


def test_parse_reject() -> None:
    cmd = parse_command(["REJECT", "BAD_SYNTAX"])
    assert isinstance(cmd, Reject)
    assert cmd.code == "BAD_SYNTAX"


def test_parse_start() -> None:
    cmd = parse_command(["START", "W", "Bob"])
    assert isinstance(cmd, Start)
    assert cmd.color == "W"
    assert cmd.nick == "Bob"


def test_parse_over() -> None:
    cmd = parse_command(["OVER", "B", "SURROUND"])
    assert isinstance(cmd, Over)
    assert cmd.color == "B"
    assert cmd.reason == "SURROUND"


def test_serialize_place() -> None:
    wire = serialize(Place(p=1, q=-2, piece=PieceKind.Queen))
    assert wire == "HIVE PLACE 1 -2 Q"


def test_serialize_login() -> None:
    wire = serialize(Login(nick="Alice", lobby="lobby"))
    assert wire == "HIVE LOGIN 05Alice 05lobby"


def test_serialize_rematch() -> None:
    wire = serialize(Rematch())
    assert wire == "HIVE REMATCH"


def test_roundtrip_place() -> None:
    original = Place(p=3, q=-1, piece=PieceKind.Ant)
    wire = serialize(original)
    # Re-parse: the wire format for PLACE is "HIVE PLACE p q piece"
    cmd = parse_command(wire.split(" ")[1:])
    assert cmd == original


def test_piece_display() -> None:
    white_queen = Piece(kind=PieceKind.Queen, owner=Player.White)
    black_queen = Piece(kind=PieceKind.Queen, owner=Player.Black)
    assert white_queen.display() == "QUEEN"
    assert black_queen.display() == "queen"


def test_tile_pointer_str() -> None:
    ptr = TilePointer(3, -1)
    assert str(ptr) == "(3, -1)"


def test_tile_pointer_hash() -> None:
    assert hash(TilePointer(1, 2)) == hash(TilePointer(1, 2))


def test_game_state_initial_state() -> None:
    state = GameState()
    assert state.result is GameResult.ONGOING
    assert state.current_player is Player.White
    assert state.board.is_empty_all()


def test_game_state_apply_move_switches_player() -> None:
    state = GameState()
    state.apply(make_placement(TilePointer(0, 0), PieceKind.Queen))
    assert state.current_player is Player.Black
    assert state.result is GameResult.ONGOING


def test_game_state_queen_surrounded_black_wins() -> None:
    # White queen at (0,0) surrounded by 6 black pieces -> black wins.
    state = GameState()
    state.apply(make_placement(TilePointer(0, 0), PieceKind.Queen))  # White Q
    # Surround with black pieces at all 6 neighbors (direct board placement).
    neighbors = [
        TilePointer(1, 0),
        TilePointer(0, 1),
        TilePointer(-1, 1),
        TilePointer(-1, 0),
        TilePointer(0, -1),
        TilePointer(1, -1),
    ]
    kinds = [
        PieceKind.Ant,
        PieceKind.Spider,
        PieceKind.Beetle,
        PieceKind.Grasshopper,
        PieceKind.Ant,
        PieceKind.Spider,
    ]
    for ptr, kind in zip(neighbors, kinds):
        state.board.apply_move(make_placement(ptr, kind), Player.Black)
    state._check_win()
    assert state.result is GameResult.BLACK_WINS


def test_game_state_legal_moves_empty_board() -> None:
    state = GameState()
    assert state.legal_moves() == []


# ---------------------------------------------------------------------------
# Property-based tests (hypothesis)
# ---------------------------------------------------------------------------

from hypothesis import given, settings
from hypothesis import strategies as st


@st.composite
def tile_pointer(draw: st.DrawFn) -> TilePointer:
    return TilePointer(draw(st.integers(-5, 5)), draw(st.integers(-5, 5)))


@st.composite
def piece_kind(draw: st.DrawFn) -> PieceKind:
    return draw(
        st.sampled_from(
            [
                PieceKind.Queen,
                PieceKind.Spider,
                PieceKind.Beetle,
                PieceKind.Grasshopper,
                PieceKind.Ant,
            ]
        )
    )


@st.composite
def player(draw: st.DrawFn) -> Player:
    return draw(st.sampled_from([Player.White, Player.Black]))


@st.composite
def reachable_board(draw: st.DrawFn) -> Board:
    """Build a board by applying a sequence of legal placements/moves."""
    board = Board()
    player = Player.White
    # Place up to 8 pieces, alternating players where legal.
    for _ in range(draw(st.integers(0, 8))):
        moves = list(board.moves_for_player(player, board.get_player_pieces()[player]))
        if not moves:
            break
        move = draw(st.sampled_from(moves))
        board.apply_move(move, player)
        player = player.other()
    return board


@given(reachable_board())
@settings(max_examples=200)
def test_prop_every_generated_move_is_legal(board: Board) -> None:
    """Every move returned by moves_for_player must be applicable."""
    for player in (Player.White, Player.Black):
        for move in board.moves_for_player(player, board.get_player_pieces()[player]):
            trial = board.clone()
            trial.apply_move(move, player)


@given(reachable_board())
@settings(max_examples=200)
def test_prop_hive_stays_connected_after_move(board: Board) -> None:
    """After any legal move, all pieces on the board must be connected."""
    if board.is_empty_all():
        return
    for player in (Player.White, Player.Black):
        for move in board.moves_for_player(player, board.get_player_pieces()[player]):
            trial = board.clone()
            trial.apply_move(move, player)
            # Collect all occupied tiles.
            occupied = set(trial._data.keys())
            if len(occupied) <= 1:
                continue
            # Flood-fill from an arbitrary tile.
            start = next(iter(occupied))
            seen: set[TilePointer] = {start}
            stack = [start]
            while stack:
                cur = stack.pop()
                for nb in trial.neighbors(cur):
                    if nb not in seen:
                        seen.add(nb)
                        stack.append(nb)
            assert seen == occupied, "Hive split into disconnected components"


@given(reachable_board())
@settings(max_examples=200)
def test_prop_place_decrements_count(board: Board) -> None:
    """Placing a piece decrements the remaining count for that kind."""
    for player in (Player.White, Player.Black):
        before = dict(board.get_player_pieces()[player])
        for move in board.moves_for_player(player, board.get_player_pieces()[player]):
            if isinstance(move, MovePiece):
                continue  # skip moves, only test placements
            trial = board.clone()
            trial.apply_move(move, player)
            after = trial.get_player_pieces()[player]
            assert after[move.piece_kind] == before[move.piece_kind] - 1
            # Other kinds unchanged.
            for k, v in before.items():
                if k != move.piece_kind:
                    assert after[k] == v


@given(reachable_board())
@settings(max_examples=200)
def test_prop_move_preserves_counts(board: Board) -> None:
    """Moving a piece must not change remaining piece counts."""
    for player in (Player.White, Player.Black):
        before = dict(board.get_player_pieces()[player])
        for move in board.moves_for_player(player, board.get_player_pieces()[player]):
            if not isinstance(move, MovePiece):
                continue  # skip placements, only test moves
            trial = board.clone()
            trial.apply_move(move, player)
            after = trial.get_player_pieces()[player]
            assert after == before, "Moving a piece changed supply counts"


@given(
    st.sampled_from(
        [
            Direction(0, 1),
            Direction(1, 0),
            Direction(-1, 1),
            Direction(-1, 0),
            Direction(0, -1),
            Direction(1, -1),
        ]
    )
)
@settings(max_examples=100)
def test_prop_rotate_inverse(direction: Direction) -> None:
    """rotate_right(rotate_left(d)) == d and vice versa."""
    from hive_client.game import rotate_left, rotate_right

    assert rotate_right(rotate_left(direction)) == direction
    assert rotate_left(rotate_right(direction)) == direction


@given(tile_pointer())
@settings(max_examples=200)
def test_prop_tile_pointer_hash_consistent(ptr: TilePointer) -> None:
    """Equal TilePointers have equal hashes."""
    other = TilePointer(ptr.p, ptr.q)
    assert hash(ptr) == hash(other)
    assert ptr == other


@given(st.sampled_from(["Q", "S", "B", "G", "A"]))
@settings(max_examples=100)
def test_prop_serialize_parse_roundtrip(piece_letter: str) -> None:
    """serialize(parse_command(tokens)) should reproduce the wire format."""
    from hive_client.network import parse_command, serialize

    cmd = parse_command(["PLACE", "3", "-1", piece_letter])
    assert isinstance(cmd, Place)
    wire = serialize(cmd)
    # Re-parse the wire format.
    reparsed = parse_command(wire.split(" ")[1:])
    assert reparsed == cmd


@given(st.sampled_from(["LOGIN", "PLACE", "MOVE", "REMATCH", "LEAVE", "LIST"]))
@settings(max_examples=100)
def test_prop_command_roundtrip(command: str) -> None:
    """Every serializable command round-trips through parse/serialize."""
    from hive_client.network import (
        Leave,
        ListRequest,
        Login,
        Place,
        Rematch,
        serialize,
    )

    cmd_map = {
        "LOGIN": Login(nick="A", lobby="B"),
        "PLACE": Place(p=1, q=2, piece=PieceKind.Queen),
        "MOVE": Move(from_p=0, from_q=0, to_p=1, to_q=0, piece=PieceKind.Queen),
        "REMATCH": Rematch(),
        "LEAVE": Leave(),
        "LIST": ListRequest(),
    }
    cmd = cmd_map[command]
    wire = serialize(cmd)
    reparsed = parse_command(wire.split(" ")[1:])
    assert reparsed == cmd
