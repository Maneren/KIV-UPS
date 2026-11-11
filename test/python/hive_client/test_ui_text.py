"""Tests for the text-wrapping helper."""

import pytest
from hive_client.ui import text


@pytest.fixture(autouse=True)
def _fake_measure(monkeypatch: pytest.MonkeyPatch) -> None:
    def fake_measure(value: str, _font_size: int) -> float:
        return float(len(value))

    monkeypatch.setattr(text, "measure_text", fake_measure)


def test_short_text_stays_on_one_line() -> None:
    assert text.wrap_text("hello world", 20.0, 10) == ["hello world"]


def test_wraps_at_word_boundaries() -> None:
    assert text.wrap_text("one two three four", 9.0, 10) == [
        "one two",
        "three",
        "four",
    ]


def test_blank_line_is_preserved() -> None:
    assert text.wrap_text("", 10.0, 10) == [""]


def test_long_word_overflows_instead_of_looping() -> None:
    assert text.wrap_text("abcdefghij", 3.0, 10) == ["abcdefghij"]


def test_bullet_continuations_are_indented() -> None:
    assert text.wrap_text("* one two three", 9.0, 10) == [
        "* one two",
        "  three",
    ]
