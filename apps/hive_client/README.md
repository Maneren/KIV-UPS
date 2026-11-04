# Hive client (Python)

uv-managed Python app, built via CMake (`cmake/PythonAppTemplate.cmake`).
GUI via the [`raylib`](https://pypi.org/project/raylib/) package (Python
bindings for raylib 6, incl. raygui) — `from pyray import ...`.

```bash
just build                   # runs `uv sync` for the client
just run Debug hive_client   # runs the generated launcher
uv run --project apps/hive_client --group dev pytest test/python  # direct test run
```

`hive_client` opens the game window (needs a display).

## Architecture

The package is split into layers so that game rules stay independent of the
GUI, the terminal, and the network:

| Package | Responsibility | Depends on |
| --- | --- | --- |
| `hive_client.domain` | Pure game rules: hex geometry, pieces, moves, board state, turn/win logic (`GameSession`). | — |
| `hive_client.app` | UI-agnostic interaction: `GameController` turns intents into validated moves; `GameView` is the immutable render model. | domain |
| `hive_client.ui` | raylib only: pixel geometry, layout, theme, input hit-testing, renderer, window loop. | app, domain |
| `hive_client.wire` | Wire protocol: message dataclasses, pure codec, TCP transport. | domain |

`hive_client.game`, `hive_client.gui`, `hive_client.protocol`, and
`hive_client.network` are thin compatibility facades over the new packages.
Importing `hive_client` itself does not import raylib; the GUI is loaded
lazily from `hive_client.cli`.

All game-rule mutations go through `GameSession.submit()`. In particular, the
GUI never sets `current_player`, computes forced passes, or checks move
legality itself.
