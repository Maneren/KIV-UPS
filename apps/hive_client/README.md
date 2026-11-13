# Hive client (Python)

uv-managed Python app, built via CMake (`cmake/PythonAppTemplate.cmake`).
GUI via the [`raylib`](https://pypi.org/project/raylib/) package (Python
bindings for raylib 6, incl. raygui) — `from pyray import ...`.

```bash
just build                   # runs `uv sync` for the client, builds the native engine
just run Debug hive_client   # runs the generated launcher
PYTHONPATH=build/python/Debug uv run --project apps/hive_client --group dev pytest test/python
```

`hive_client` opens the game window (needs a display).

## Architecture

Game rules live in C++ (`src/hive`) and are exposed to Python through the
`hive_native` extension module. The Python package is split into layers so that
the GUI, the terminal, and the network stay independent of the engine:

| Package | Responsibility | Depends on |
| --- | --- | --- |
| `hive_native` | Canonical Hive engine: board state, legal moves, turn/win logic. | — |
| `hive_client.domain` | Value objects shared by the app, UI, and wire layers. | `hive_native` |
| `hive_client.app` | UI-agnostic interaction: `GameController` calls `hive_native.Game` directly; `GameView` is the immutable render model. | domain, `hive_native` |
| `hive_client.ui` | raylib only: pixel geometry, layout, theme, input hit-testing, renderer, window loop. | app, domain |
| `hive_client.wire` | Wire protocol: message dataclasses, pure codec, TCP transport. | domain |

Importing `hive_client` itself does not import raylib; the GUI is loaded
lazily from `hive_client.cli`.

All game-rule mutations go through `hive_native.Game.submit()`. In particular,
the GUI never sets `current_player`, computes forced passes, or checks move
legality itself; those queries are answered by the native engine.
