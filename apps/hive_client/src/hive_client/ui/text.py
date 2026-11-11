"""Text wrapping helpers for the raylib UI."""

from __future__ import annotations

from pyray import measure_text

__all__ = ["wrap_text"]


def wrap_text(
    text: str,
    max_width: float,
    font_size: int,
    *,
    continuation_indent: str = "  ",
) -> list[str]:
    """
    Split ``text`` into lines no wider than ``max_width``.

    ``measure`` returns the rendered width of a string. Lines starting with a
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
