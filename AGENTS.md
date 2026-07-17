# AGENTS.md

Guidance for AI coding agents working in this repository.

## Project Style

- Keep this a modern C++17 project.
- Follow Google C++ style and the checked-in `.clang-format`.
- Prefer clear, simple C++17 code over legacy C++11 patterns.
- Keep naming consistent with nearby code.
- Use `Create()` for owning factory functions and return `std::unique_ptr`
  instead of owning raw pointers.
- Prefer concise accessor names such as `Type()`, `Encoding()`, and
  `TotalBytes()` over Java-style `Get...` names for new or renamed APIs.
- Prefer `std::string_view` for read-only string inputs and visitor-style
  traversal for hot paths; keep vector-returning helpers for convenience APIs
  and tests, not command execution paths that can stream replies directly.
- Command handlers should read arguments through `CommandArgs`
  (`std::string_view`s into the client query buffer). Copy arguments only at an
  ownership boundary, such as storing a key/value in the DB or data structure.
- Keep request decoding incremental and zero-copy. Standard RESP arrays are the
  primary wire format; inline commands remain a compatibility path.
- Keep protocol-sensitive replies based on the client's negotiated RESP
  version. Connections default to RESP2 and may switch through `HELLO`.
- Pass `CommandArgs` by const reference instead of copying its view vector.
  Move completed `std::string` replies into `Client::AddReply`; use the
  `std::string_view` overload only for borrowed reply data.
- Add comments only when they clarify non-obvious behavior.
- Keep command handler declarations grouped in `server/commands/handlers.h`;
  avoid per-command headers unless a handler becomes a broader shared API.
- When adding or changing supported commands, update the README command
  coverage list and the relevant command-family integration test.
- Keep reply encoding helpers under `server/reply/`, and keep database state
  and Redis object wrappers under `server/db/`.

## Build And Test

Use CMake presets:

```sh
cmake --preset debug
cmake --build --preset debug
```

Run unit and integration tests separately:

```sh
ctest --preset debug -L unit --output-on-failure
ctest --preset debug -L integration --output-on-failure
```

When changing behavior shared with Redis and a reference server is available,
run:

```sh
scripts/run_redis_compatibility_check.sh build/debug/redis_simple
```

Run release and sanitizer checks after changes to ownership, memory layout,
assertions, or low-level data structures:

```sh
cmake --preset release
cmake --build --preset release
ctest --preset release --output-on-failure
cmake --preset sanitizer
cmake --build --preset sanitizer
ctest --preset sanitizer --output-on-failure
scripts/run_leak_check.sh
```

The leak-check script uses Apple `leaks` for the macOS unit-test binary and
explicit LeakSanitizer options for the complete Linux sanitizer test suite.

Use Docker for a local Linux build and test check from macOS:

```sh
scripts/run_linux_docker_check.sh
```

The helper prepares its cached Ubuntu toolchain without mounting the
repository. Its build and test container has networking disabled and mounts the
repository read-only.

Before committing, run the relevant build and tests.

## Test Layout

- Unit tests stay colocated with implementation files as `*_test.cpp`.
- Register unit tests in CTest by GoogleTest suite through
  `redis_simple_add_gtest_suite`, so failures identify the affected suite while
  preserving same-suite fixture behavior.
- Integration tests live under `integration/`.
- Current integration coverage should stay focused:
  - `integration/commands/`
  - `integration/tcp/`
- Keep command-family integration tests split by area, including key, string,
  set, list, zset, and hash commands.
- Register integration command tests as separate CTest entries by command
  family, so failures identify the affected area without log digging.
- Keep exact ordering assertions for ordered command results such as sorted-set
  ranges; do not sort actual output in compatibility tests.
- Project runner scripts live under `scripts/`.
- Do not add manual log-inspection tests. Tests should assert behavior and
  return nonzero on failure.

## Project Management

- Run `clang-format` on changed C/C++ files before committing or pushing.
- Use `scripts/format.sh --check` and `scripts/run_clang_tidy.sh` for local
  quality checks; clang-tidy warnings are treated as errors.
- Never put required side effects inside `assert`; release builds must preserve
  behavior when assertions are disabled.
- Always update relevant docs, including `README.md` and this `AGENTS.md`, when
  changing build, test, workflow, or project conventions.
- Keep CMake target-based. Source files are discovered by scoped directory
  globs in `CMakeLists.txt`; exclude generated, test, or entry-point sources
  explicitly when they do not belong in a library target.
- Keep event-loop pollers platform-selected in `CMakeLists.txt`: `kqueue` for
  macOS and `epoll` for Linux.
- Keep `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/build.yml`,
  `.clang-format`, `.clang-tidy`, and `.editorconfig` aligned with project
  conventions.
- Avoid unrelated refactors while making focused changes.
- Do not reintroduce stale mock targets that are not part of CTest or normal
  project workflows.
