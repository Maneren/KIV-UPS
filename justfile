root := justfile_directory()
builddir := root / "build"
export CMAKE_EXPORT_COMPILE_COMMANDS := "1"
export CMAKE_COLOR_DIAGNOSTICS := "ON"

# Print stdout and stderr on failed tests

export CTEST_OUTPUT_ON_FAILURE := "1"

# Number of threads to use for running tests (0 is auto)

export CTEST_PARALLEL_LEVEL := "0"

# Python state (venv, pycache, tool caches) lives in the build tree.
# Keep in sync with cmake/PythonAppTemplate.cmake.
export UV_PROJECT_ENVIRONMENT := builddir / "venvs" / "hive_client" / ".venv"
export PYTHONPYCACHEPREFIX := builddir / "pycache"
export RUFF_CACHE_DIR := builddir / "ruff-cache"

[private]
default:
    @just --list

configure build_system="Ninja Multi-Config" +args="": clean
    cmake -B {{ builddir }} -G "{{ build_system }}" -Wno-experimental {{ args }}

build config="Debug" target="all":
    cmake --build {{ builddir }} --config {{ config }} --target {{ target }}

run config executable +args="": (build config executable)
    '{{ builddir }}/bin/{{ config }}/{{ executable }}' {{ args }}

test config="Debug": (build config "all_tests")
    ctest --test-dir {{ builddir }} -C {{ config }} -j {{ CTEST_PARALLEL_LEVEL }}

lint: lint-cpp lint-py

lint-cpp:
    clang-tidy -p {{ builddir }} --quiet $(find src apps test -type f \( -name "*.cpp" -or -name "*.h" -or -name "*.hpp" \) -not -path "**/external/*")

lint-py:
    uv run --project apps/hive_client --group dev ruff check apps test/python
    uv run --project apps/hive_client --group dev ty check apps test/python

format: format-cpp format-py

format-cpp:
    clang-format -i $(find src apps test -type f \( -name "*.cpp" -or -name "*.h" -or -name "*.hpp" \) -not -path "*/external/*")
    gersemi -i . --definitions cmake --no-cache

format-py:
    uv run --project apps/hive_client --group dev ruff format apps test/python
    uv run --project apps/hive_client --group dev ruff check --fix apps test/python

clean:
    rm -rf build
