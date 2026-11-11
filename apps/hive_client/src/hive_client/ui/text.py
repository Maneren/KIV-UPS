"""Text layout and drawing helpers for the raylib UI."""

from __future__ import annotations

from pyray import Color, draw_text, measure_text

from hive_client.ui.geometry import Point

__all__ = [
    "draw_text_centered",
    "draw_text_centered_x",
    "draw_text_centered_y",
    "wrap_text",
]


def wrap_text(
    text: str,
    max_width: float,
    font_size: int,
    *,
    continuation_indent: str = "  ",
) -> list[str]:
    """
    Split ``text`` into lines no wider than ``max_width``.

    Widths are measured with raylib's ``measure_text``. Lines starting with a
    ``"* "`` bullet indent their continuations to align with the bullet text.
    """
    if not text:
        return [""]

    indent = continuation_indent if text.startswith("* ") else ""
    lines: list[str] = []
    current = ""

    for word in text.split():
        candidate = word if not current else f"{current} {word}"
        if not current or measure_text(candidate, font_size) <= max_width:
            current = candidate
        else:
            lines.append(current)
            current = indent + word

    lines.append(current)
    return lines


def draw_text_centered(
    text: str,
    center: Point,
    font_size: int,
    color: Color,
) -> None:
    """Draw ``text`` centred on ``center``."""
    draw_text(
        text,
        int(center[0] - measure_text(text, font_size) / 2.0),
        int(center[1] - font_size / 2.0),
        font_size,
        color,
    )


def draw_text_centered_x(
    text: str,
    center_x: float,
    y: float,
    font_size: int,
    color: Color,
) -> None:
    """Draw ``text`` horizontally centred on ``center_x``, top-aligned at ``y``."""
    draw_text(
        text,
        int(center_x - measure_text(text, font_size) / 2.0),
        int(y),
        font_size,
        color,
    )


def draw_text_centered_y(
    text: str,
    x: float,
    center_y: float,
    font_size: int,
    color: Color,
) -> None:
    """Draw ``text`` vertically centred on ``center_y``, left-aligned at ``x``."""
    draw_text(text, int(x), int(center_y - font_size / 2.0), font_size, color)
