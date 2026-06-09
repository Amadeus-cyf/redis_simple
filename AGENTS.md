# AGENTS.md

Guidance for AI coding agents working in this repository.

## Project Style

- Keep this a modern C++17 project.
- Follow Google C++ style and the checked-in `.clang-format`.
- Prefer clear, simple C++17 code over legacy C++11 patterns.
- Keep naming consistent with nearby code.
- Add comments only when they clarify non-obvious behavior.

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

Before committing, run the relevant build and tests.

## Test Layout

- Unit tests stay colocated with implementation files as `*_test.cpp`.
- Register unit tests in CTest by GoogleTest suite through
  `redis_simple_add_gtest_suite`, so failures identify the affected suite while
  preserving same-suite fixture behavior.
- Integration tests live under `integration/`.
- Current integration coverage should stay focused:
  - `integration/command/`
  - `integration/tcp/`
- Register integration command tests as separate CTest entries by command
  family, so failures identify the affected area without log digging.
- Project runner scripts live under `scripts/`.
- Do not add manual log-inspection tests. Tests should assert behavior and
  return nonzero on failure.

## Project Management

- Run `clang-format` on changed C/C++ files before committing or pushing.
- Always update relevant docs, including `README.md` and this `AGENTS.md`, when
  changing build, test, workflow, or project conventions.
- Keep `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/build.yml`,
  `.clang-format`, `.clang-tidy`, and `.editorconfig` aligned with project
  conventions.
- Avoid unrelated refactors while making focused changes.
- Do not reintroduce stale mock targets that are not part of CTest or normal
  project workflows.
