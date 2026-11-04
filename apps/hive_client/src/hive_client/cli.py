"""Command-line entry point for the Hive client."""

from __future__ import annotations

import argparse
import contextlib
from collections.abc import Sequence

__all__ = ["DEFAULT_HOST", "DEFAULT_PORT", "main"]

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8080


def main(argv: Sequence[str] | None = None) -> int:
    """Parse arguments and run the Hive client."""
    parser = argparse.ArgumentParser(
        prog="hive_client",
        description="Hive game client (Python)",
    )
    _ = parser.add_argument("--host", default=DEFAULT_HOST, help="Server host")
    _ = parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help="Server port",
    )
    args = parser.parse_args(argv)
    host: str = args.host
    port: int = args.port

    # Import the GUI lazily so ``--help`` and headless imports do not need
    # raylib or a display.
    from hive_client.ui.app import run  # noqa: PLC0415 - lazy GUI import

    with contextlib.suppress(KeyboardInterrupt):
        run(host, port)

    return 0
