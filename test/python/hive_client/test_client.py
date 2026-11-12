"""Tests for Hive value objects and the wire protocol."""

from hive_client.domain import (
    Direction,
    Piece,
    PieceKind,
    Player,
    TilePointer,
    rotate_left,
    rotate_right,
)
from hive_client.wire import (
    Leave,
    ListRequest,
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


# ---------------------------------------------------------------------------
# Property-based tests (hypothesis)
# ---------------------------------------------------------------------------

from hypothesis import given, settings
from hypothesis import strategies as st


@st.composite
def tile_pointer(draw: st.DrawFn) -> TilePointer:
    return TilePointer(draw(st.integers(-5, 5)), draw(st.integers(-5, 5)))


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
