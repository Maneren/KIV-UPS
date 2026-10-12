"""Hive game client."""

import argparse
from collections.abc import Sequence

from hive_client.gui import run

__all__ = ["main"]

DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8080


def main(argv: Sequence[str] | None = None) -> int:
    """Run the Hive client."""
    parser = argparse.ArgumentParser(
        prog="hive_client",
        description="Hive game client (Python)",
    )
    _ = parser.add_argument("--host", default=DEFAULT_HOST, help="Server host")
    _ = parser.add_argument(
        "--port", type=int, default=DEFAULT_PORT, help="Server port"
    )
    args = parser.parse_args(argv)
    host: str = args.host
    port: int = args.port

    run(host=host, port=port)
    return 0
