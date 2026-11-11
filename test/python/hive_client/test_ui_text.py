"""Tests for the pure text-wrapping helper (no raylib required)."""

from hive_client.ui.text import wrap_text


def _measure(text: str) -> float:
    return float(len(text))


def test_short_text_stays_on_one_line() -> None:
    assert wrap_text("hello world", 20.0, _measure) == ["hello world"]


def test_wraps_at_word_boundaries() -> None:
    assert wrap_text("one two three four", 9.0, _measure) == [
        "one two",
        "three",
        "four",
    ]


def test_blank_line_is_preserved() -> None:
    assert wrap_text("", 10.0, _measure) == [""]


def test_long_word_overflows_instead_of_looping() -> None:
    assert wrap_text("abcdefghij", 3.0, _measure) == ["abcdefghij"]


def test_bullet_continuations_are_indented() -> None:
    assert wrap_text("* one two three", 9.0, _measure) == [
        "* one two",
        "  three",
    ]
