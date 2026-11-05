# kiv-ups

Implementation of the board game **Hive**: a C++ game engine and networked
server (`src/`) with a Python/raylib client (`apps/hive_client`).

## Rules of Hive

Hive is a two-player abstract strategy game played with hexagonal tiles
representing various insects. There is no board: tiles are placed adjacent to
one another, forming a shared hive. The goal is to completely surround your
opponent's Queen Bee.

- Each player starts with the same set of tiles: one Queen Bee, two Ants, two
  Spiders, two Beetles, and three Grasshoppers.
- On your turn you either place a new tile from your hand into the hive, or
  move a tile already in play.
- The Queen Bee must be placed within your first four turns.
- Each insect moves differently:
  - Queen Bees move one space at a time
  - Ants move anywhere around the hive
  - Spiders move around the hive, but only exactly three spaces
  - Beetles move one space at a time, and can climb on top of other insects
  - Grasshoppers jump in a straight line to the first empty space
- An insect may only be moved if:
  - it does not break the hive into two separate pieces,
  - it is the top-most insect at its tile, and
  - it physically fits through the gaps between adjacent tiles
    (except for Beetles and Grasshoppers which physically move over
    other insects).
- You win by surrounding the enemy Queen Bee with tiles (of either color).

See the [Wikipedia article](<https://en.wikipedia.org/wiki/Hive_(game)>) for the
full rules.

## How to run

### Prerequisites

- A C++23 compiler (GCC or Clang)
- CMake 3.31+
- Ninja
- [just](https://github.com/casey/just)
- [uv](https://docs.astral.sh/uv/) (for the Python client)

### Build

```bash
just configure   # configure the build in ./build
just build       # compile everything (Debug)
```

Without `just`:

```bash
cmake -B build -G "Ninja Multi-Config"
cmake --build build --config Debug
```

### Run

```bash
just run Debug hive_server   # C++ server
just run Debug hive_client   # Python client (requires a display)
```

### Test

```bash
just test
```
