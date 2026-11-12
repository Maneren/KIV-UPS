"""
Exceptions raised by the Hive domain layer.

The exception types come from the native engine; :class:`HiveError` derives
from :class:`RuntimeError`.
"""

from __future__ import annotations

from hive_native import EmptyTileError, HiveError, IllegalMoveError

__all__ = ["EmptyTileError", "HiveError", "IllegalMoveError"]
