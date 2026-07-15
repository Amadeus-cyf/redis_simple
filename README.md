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
- macOS (`kqueue`) or Linux (`epoll`) for the event-loop poller

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

## Command Coverage

The server supports Redis-style RESP replies for the implemented command set,
including protocol errors such as `ERR wrong number of arguments` and
`WRONGTYPE`.

- Keys: `DEL`, `UNLINK`, `EXISTS`, `TYPE`, `EXPIRE`, `PEXPIRE`, `TTL`, `PTTL`,
  `PERSIST`, `RENAME`, `DBSIZE`, `FLUSHDB`
- Strings: `GET`, `SET` with `EX`, `PX`, and `KEEPTTL`, `INCR`, `DECR`,
  `APPEND`, `MGET`, `MSET`
- Lists: `LPUSH`, `RPUSH`, `LPOP`, `RPOP`, `LLEN`, `LRANGE`, `LINDEX`, `LSET`,
  `LREM`, `LTRIM`
- Sets: `SADD`, `SCARD`, `SREM`, `SMEMBERS`, `SISMEMBER`, `SINTER`, `SUNION`,
  `SDIFF`
- Sorted sets: `ZADD`, `ZCARD`, `ZREM`, `ZRANK`, `ZRANGE`, `ZREVRANGE`,
  `ZRANGEBYSCORE`, `ZCOUNT`, `ZSCORE`
- Hashes: `HSET`, `HGET`, `HDEL`, `HLEN`, `HEXISTS`, `HGETALL`, `HMGET`,
  `HKEYS`, `HVALS`, `HINCRBY`

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

Release and sanitizer presets are available for checking behavior with
assertions disabled and for detecting memory or undefined-behavior defects:

```sh
cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure

cmake --preset sanitizer
cmake --build --preset sanitizer
ctest --preset sanitizer --output-on-failure
```

This runs:

- `redis_simple_unit_<SuiteName>`: unit tests compiled into
  `redis_simple_tests` and registered in CTest by GoogleTest suite.
- `redis_simple_integration_tcp`: TCP client/server integration check.
- `redis_simple_integration_command_key`: generic key command integration
  checks.
- `redis_simple_integration_command_string`: string command integration checks.
- `redis_simple_integration_command_set`: set command integration checks.
- `redis_simple_integration_command_list`: list command integration checks.
- `redis_simple_integration_command_zset`: sorted-set command integration
  checks.
- `redis_simple_integration_command_hash`: hash command integration checks.

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
cli/                  Simple client and RESP parsing
connection/           Connection abstraction
event_loop/           Event loop with kqueue and epoll pollers
integration/commands/ Server/client command integration tests
integration/tcp/      TCP client/server integration tests
logging/              Project logging wrapper
memory/               Core in-memory data structures
scripts/              Project automation and CTest runner scripts
server/               Server, client connection glue, commands, replies, DB
data_types/           Redis hash, list, set, and sorted-set implementations
tcp/                  TCP helpers
utils/                Small shared utilities
benchmarks/           Memory/data-structure benchmarks
```

Command handler declarations are grouped in `server/commands/handlers.h`; the
individual command `.cpp` files keep implementation details local.
Reply encoding helpers live under `server/reply/`, while database state and
Redis object wrappers stay under `server/db/`.

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

Check formatting without modifying files with:

```sh
scripts/format.sh --check
```

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
are excluded from library targets explicitly. Platform-specific event-loop
pollers are selected at configure time: `kqueue` on macOS and `epoll` on Linux.

Run the Linux build and test flow locally from macOS through Docker:

```sh
scripts/run_linux_docker_check.sh
```

On its first run, the Docker helper creates a local Ubuntu 24.04 toolchain image
without mounting the repository. Build and test runs then disable container
networking, mount the repository read-only, drop Linux capabilities, and build
under a temporary filesystem. This validates the Linux `epoll` poller without
writing Linux artifacts into the working tree or exposing source code to the
container network. Set `DOCKER_BASE_IMAGE`, `DOCKER_TOOLCHAIN_IMAGE`, or
`LINUX_DOCKER_BUILD_DIR` to override the defaults.

## C++ Style

The code follows Google C++ style with project-local conventions: owning factory
functions are named `Create()` and return `std::unique_ptr`, while concise
accessors such as `Type()`, `Encoding()`, and `TotalBytes()` are preferred over
Java-style `Get...` names.

Hot paths prefer `std::string_view` for read-only inputs and visitor-style
traversal when callers can encode or rebuild results directly. Vector-returning
helpers remain useful for convenience APIs and tests, but command handlers
should avoid materializing extra copies when they can stream replies in one
pass.

## CI

GitHub Actions runs the same flow recommended locally on macOS and Ubuntu, so
both event-loop pollers are built and tested. Separate Ubuntu jobs also enforce
clang-format and clang-tidy, run the full release suite, and run ASan/UBSan:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug -L unit --output-on-failure
ctest --preset debug -L integration --output-on-failure
```
