"""
Hive game client.

Importing this package is side-effect free and does not import raylib; the GUI
is loaded lazily by :func:`hive_client.cli.main`.
"""

from hive_client.cli import DEFAULT_HOST, DEFAULT_PORT, main

__all__ = ["DEFAULT_HOST", "DEFAULT_PORT", "main"]
