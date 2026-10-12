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
