"""Blocking TCP transport for the Hive protocol."""

from __future__ import annotations

import socket

from hive_client.wire.codec import parse_command, serialize
from hive_client.wire.messages import MAGIC, Command, IncomingMessage

__all__ = ["HiveClient"]


class HiveClient:
    """A TCP connection to a Hive server with buffered line-based I/O."""

    def __init__(self, host: str, port: int) -> None:
        """Create a client for ``host``:``port`` (connection is lazy)."""
        self._host = host
        self._port = port
        self._sock: socket.socket | None = None
        self._buffer = ""

    def connect(self, timeout: float = 5.0) -> None:
        """Open a TCP connection to the server."""
        self._sock = socket.create_connection((self._host, self._port), timeout)
        self._sock.settimeout(None)  # blocking after connect

    def close(self) -> None:
        """Close the TCP connection."""
        if self._sock is not None:
            self._sock.close()
            self._sock = None

    def send(self, cmd: Command) -> None:
        """Send a command to the server."""
        if self._sock is None:
            raise RuntimeError("not connected")
        line = serialize(cmd) + "\n"
        self._sock.sendall(line.encode("ascii"))

    def receive(self) -> IncomingMessage:
        """Block until one full message is available and parse it."""
        if self._sock is None:
            raise RuntimeError("not connected")

        while "\n" not in self._buffer:
            chunk = self._sock.recv(4096)
            if not chunk:
                raise ConnectionError("server closed the connection")
            self._buffer += chunk.decode("ascii")

        line, self._buffer = self._buffer.split("\n", 1)
        tokens = line.split(" ")
        if not tokens or tokens[0] != MAGIC:
            raise ValueError(f"bad magic: {tokens[0] if tokens else None!r}")
        return parse_command(tokens[1:])
