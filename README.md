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
ctest --preset debug -L unit --output-on-failure
ctest --preset debug -L integration --output-on-failure
```

This runs:

- `redis_simple_unit_<SuiteName>`: unit tests compiled into
  `redis_simple_tests` and registered in CTest by GoogleTest suite.
- `redis_simple_integration_tcp`: TCP client/server integration check.
- `redis_simple_integration_command_string`: string command integration checks.
- `redis_simple_integration_command_set`: set command integration checks.
- `redis_simple_integration_command_list`: list command integration checks.
- `redis_simple_integration_command_zset`: sorted-set command integration
  checks.

For debugging a single executable directly:

```sh
./build/debug/redis_simple_tests
./build/debug/mock_set_client
```

For debugging one unit-test suite directly:

```sh
./build/debug/redis_simple_tests --gtest_filter=ListPackTest.*
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
connection/          Connection abstraction
event_loop/          Event loop and kqueue backend
integration/command/ Server/client command integration tests
integration/tcp/     TCP client/server integration tests
logging/             Project logging wrapper
memory/              Core in-memory data structures
scripts/             Project automation and CTest runner scripts
server/              Server, client connection glue, commands, replies, DB
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

Format project C++ files with:

```sh
cmake --build --preset debug --target format
```

For one-off formatting, `scripts/format.sh` discovers Homebrew's LLVM toolchain
on macOS and honors `CLANG_FORMAT_BIN` when set.

Run clang-tidy with the project wrapper:

```sh
scripts/run_clang_tidy.sh
```

The wrapper uses `build/debug/compile_commands.json` by default and discovers
Homebrew's LLVM toolchain on macOS. Set `BUILD_DIR`, `CLANG_TIDY_BIN`, or
`RUN_CLANG_TIDY_BIN` to override those defaults.

The root `CMakeLists.txt` is target-based. Production, test, and benchmark
sources are discovered from scoped project directories with
`GLOB_RECURSE CONFIGURE_DEPENDS`, while test files and executable entry points
are excluded from library targets explicitly.

## CI

GitHub Actions runs the same flow recommended locally:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug -L unit --output-on-failure
ctest --preset debug -L integration --output-on-failure
```
