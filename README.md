# redis_simple

`redis_simple` is a small Redis-inspired server implemented in modern C++17. It
includes a TCP server, event loop, command handlers, in-memory data structures,
a simple CLI client, unit tests, integration tests, and memory benchmarks.

The project is intentionally compact: implementation-level unit tests live next
to the code they cover, while runnable end-to-end checks live under
`integration/`.

## Requirements

- CMake 3.21+
- A C++17 compiler
- Git submodules for third-party dependencies
- macOS for the current event-loop backend (`kqueue`)

## Setup

```sh
git submodule update --init --recursive
cmake --preset debug
cmake --build --preset debug
```

The debug preset builds into `build/debug`. A release preset is also available:

```sh
cmake --preset release
cmake --build --preset release
```

## Run

Start the server:

```sh
./build/debug/redis_simple
```

In another terminal, run a mock command client or your own TCP client against
`localhost:8080`.

## Test

Run the full test suite through CTest:

```sh
ctest --preset debug
```

Run unit and integration tests separately:

```sh
ctest --preset debug -L unit
ctest --preset debug -L integration
```

This runs:

- `redis_simple_tests`: unit tests compiled from `*_test.cpp` files colocated
  with their implementations.
- `redis_simple_integration_tcp`: TCP client/server integration check.
- `redis_simple_integration_commands`: command-level server/client integration
  checks for strings, sets, and sorted sets.

For debugging a single executable directly:

```sh
./build/debug/redis_simple_tests
./build/debug/mock_t_set_client
```

The integration clients return nonzero on failed expectations, so they are safe
to run in CI instead of relying on manual log inspection.

## Benchmarks

```sh
./build/debug/memory_benchmark
```

Google Benchmark's own tests are disabled in this project; the benchmark target
above is the project benchmark executable.

## Project Layout

```text
cli/          Simple client and RESP parsing
connection/   Connection abstraction
event_loop/          Event loop and kqueue backend
integration/command/ Server/client command integration tests
integration/tcp/     TCP client/server integration tests
logging/             Project logging wrapper
memory/              Core in-memory data structures
scripts/             Project automation and CTest runner scripts
server/              Server, database objects, command handlers, replies
storage/             Redis-like higher-level storage types
tcp/                 TCP helpers
utils/               Small shared utilities
benchmarks/          Memory/data-structure benchmarks
```

## Tooling

Project management files are checked in:

- `CMakePresets.json` for reproducible configure/build/test commands.
- `.clang-format` for Google-style formatting.
- `.clang-tidy` for static-analysis defaults.
- `.editorconfig` for editor consistency.
- `.github/workflows/build.yml` for CI on push and pull request.
- `AGENTS.md` for AI coding-agent project guidance.

Format changed C++ files with:

```sh
clang-format -i path/to/file.cpp path/to/file.h
```

## CI

GitHub Actions runs the same flow recommended locally:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug -L unit
ctest --preset debug -L integration
```
