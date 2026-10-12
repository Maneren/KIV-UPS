"""Hive game client."""

import argparse
import sys
from collections.abc import Sequence

__all__ = ["main"]


def main(argv: Sequence[str] | None = None) -> int:
    """Run the Hive client (stub)."""
    parser = argparse.ArgumentParser(
        prog="hive_client",
        description="Hive game client (Python)",
    )
    _ = parser.add_argument("--host", default="127.0.0.1", help="Server host")
    _ = parser.add_argument("--port", type=int, default=8080, help="Server port")
    args = parser.parse_args(argv)
    host: str = args.host
    port: int = args.port

    sys.stdout.write(f"hive_client: connecting to {host}:{port} (stub)\n")
    return 0
