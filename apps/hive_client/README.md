# Hive client (Python)

uv-managed Python app, built via CMake (`cmake/PythonAppTemplate.cmake`).

```bash
just build                   # runs `uv sync` for the client
just run Debug hive_client   # runs the generated launcher
uv run --project apps/hive_client --group dev pytest test/python  # direct test run
```
