"""Tests for the pure pixel/hex conversion helpers (no raylib required)."""

from hive_client.domain.geometry import TilePointer
from hive_client.ui.geometry import hex_to_pixel, pixel_to_tile, snap_to_pixel


def test_pixel_roundtrip_for_hex_centres() -> None:
    """Converting a tile to pixels and back returns the same tile."""
    center = (400.0, 300.0)
    for tile in (
        TilePointer(0, 0),
        TilePointer(1, 0),
        TilePointer(-2, 3),
        TilePointer(4, -2),
    ):
        pixel = hex_to_pixel(tile, center)
        assert pixel_to_tile(pixel, center) == tile


def test_snap_to_pixel_is_idempotent_at_centres() -> None:
    """Snapping an exact centre does not move it."""
    center = (400.0, 300.0)
    pixel = hex_to_pixel(TilePointer(3, -1), center)

    snapped = snap_to_pixel(pixel, center)

    assert snapped == pixel


def test_snap_chooses_nearest_hex() -> None:
    """A point close to a centre still belongs to that hex."""
    center = (400.0, 300.0)
    tile = TilePointer(2, 1)
    exact = hex_to_pixel(tile, center)

    slightly_off = (exact[0] + 2.0, exact[1] - 2.0)

    assert pixel_to_tile(slightly_off, center) == tile
