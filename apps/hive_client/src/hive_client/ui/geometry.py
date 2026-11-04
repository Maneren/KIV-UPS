"""
Screen-space hex geometry.

Pure math only: no raylib imports, so this module can be tested on headless
machines.  Callers deal in plain ``(x, y)`` tuples and domain ``TilePointer``
values; the renderer adapts them to raylib vectors.
"""

from __future__ import annotations

import math

from hive_client.domain.geometry import HEX_SIDES, TilePointer

__all__ = [
    "HEX_ANGLE_STEP",
    "HEX_SIZE",
    "HEX_X_FACTOR",
    "HEX_Y_FACTOR",
    "Point",
    "hex_corners",
    "hex_to_pixel",
    "pixel_to_tile",
    "snap_to_pixel",
]

Point = tuple[float, float]

HEX_SIZE = 40.0
HEX_ANGLE_STEP = math.pi / 3.0
HEX_X_FACTOR = 1.5
HEX_Y_FACTOR = math.sqrt(3.0)


def hex_to_pixel(
    tile: TilePointer,
    center: Point,
    size: float = HEX_SIZE,
) -> Point:
    """Convert axial ``(p, q)`` coordinates to a screen point."""
    x = HEX_X_FACTOR * tile.p
    y = HEX_Y_FACTOR * (tile.q + tile.p / 2.0)
    return (center[0] + x * size, center[1] + y * size)


def pixel_to_tile(
    point: Point,
    center: Point,
    size: float = HEX_SIZE,
) -> TilePointer:
    """
    Return the hex tile containing a screen point.

    Uses cube-coordinate rounding (the standard approach for hex grids) so
    boundaries are consistent and never snap to the wrong tile.
    """
    dx = point[0] - center[0]
    dy = point[1] - center[1]

    p = dx / (HEX_X_FACTOR * size)
    q = dy / (HEX_Y_FACTOR * size) - p / 2.0

    # Axial (p, q) -> cube (x, y, z) with y = -x - z.
    x, z = p, q
    y = -x - z

    rx = round(x)
    ry = round(y)
    rz = round(z)

    x_diff = abs(rx - x)
    y_diff = abs(ry - y)
    z_diff = abs(rz - z)

    if x_diff > y_diff and x_diff > z_diff:
        rx = -ry - rz
    elif y_diff > z_diff:
        ry = -rx - rz
    else:
        rz = -rx - ry

    return TilePointer(rx, rz)


def snap_to_pixel(
    point: Point,
    center: Point,
    size: float = HEX_SIZE,
) -> Point:
    """Snap a screen point to the centre of its nearest hex tile."""
    return hex_to_pixel(pixel_to_tile(point, center, size), center, size)


def hex_corners(center: Point, size: float = HEX_SIZE) -> tuple[Point, ...]:
    """Return the six corner points of a pointy-top hexagon."""
    return tuple(
        (
            center[0] + size * math.cos(i * HEX_ANGLE_STEP),
            center[1] + size * math.sin(i * HEX_ANGLE_STEP),
        )
        for i in range(HEX_SIDES)
    )
