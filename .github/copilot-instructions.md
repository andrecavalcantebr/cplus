# cplus — Workspace Instructions

Experimental C transpiler: `.cplus`/`.hplus` source → C23 output.
Active branch: `feat/v2`. `main` is frozen at v1 DoD.

## Build and Test

```bash
# Configure (Debug — ASan/UBSan enabled)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Run tests
ctest --test-dir build --output-on-failure

# Valgrind memcheck
cmake --build build --target memcheck
```

CMake options: `CPLUS_BUILD_TESTS` (ON), `CPLUS_WARNINGS_AS_ERRORS` (OFF), `CPLUS_ENABLE_SANITIZERS` (ON in Debug).

> **After adding any new `.c` file**, reconfigure first (`cmake -B build`) — source collection uses `GLOB_RECURSE CONFIGURE_DEPENDS`.

## Architecture

See [docs/architecture.md](docs/architecture.md) for full module descriptions.

| Module | Files | Role |
|--------|-------|------|
| driver | `src/main.c` | CLI arg parsing, builds `PipelineOptions`, calls `pipeline_run()` |
| pipeline | `src/pipeline.{h,c}` | Orchestrate: load → validate → identity copy (v1) / lower (v2) |
| compiler_validator | `src/compiler_validator.{h,c}` | Runs `gcc`/`clang -fsyntax-only` via `system(3)`; GCC <14 shim maps `-std=c23→-std=c2x` |
| diagnostics | `src/diagnostics.{h,c}` | Parses raw compiler output into `DiagnosticList`; caller must call `diagnostics_free_list()` |
| strbuf | `src/strbuf.{h,c}` | Growable byte buffer ADT — `strbuf_append_bytes`, `strbuf_append_cstr`, `strbuf_take` |

**v2 modules (in progress):**

| Module | Files | Role |
|--------|-------|------|
| lexer | `src/lexer.{h,c}` | Island scanner; keyword table; pull-based `lexer_next`/`lexer_peek` |
| ast | `src/ast.{h,c}` | Sparse island AST: cplus nodes + `NODE_PASSTHROUGH` for verbatim C23 |
| parser | `src/parser.{h,c}` | Recursive descent; produces AST; detects E201/E202/E204/E205/E206 |
| lowering | `src/lowering.{h,c}` | AST visitor; emits C23 output via `StrBuf`; detects E203 |

`cplus_core` is a static library (all `src/*.c` except `main.c`). Tests link against it directly. `CPLUS_FIXTURES_DIR` is a compile-time define injected by CMake — always build via CMake, never by hand.

## Code Style

Full conventions in [docs/coding-style.md](docs/coding-style.md) and warning flags in [docs/build-flags.md](docs/build-flags.md). Critical rules:

**Language:** C23 strict (`-std=c23`, `CMAKE_C_EXTENSIONS OFF`). `strdup`/`strndup` are C23 standard — no `_POSIX_C_SOURCE` in library/src code. Test files that need POSIX APIs (e.g. `unlink`) must place `#define _POSIX_C_SOURCE 200809L` **before the file header comment**.

**File header** (required on every `.c`/`.h`):
```c
/*
 * FILE: filename.c
 * DESC.: ...
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */
```

**Include guards:** `CPLUS_NAME_H` (no leading `__`).

**Include order:** own `.h` first → stdlib → project → third-party.

**Naming:** `snake_case` for functions/variables, `PascalCase` for structs, `UPPER_SNAKE_CASE` for macros. Public symbols prefixed by module: `pipeline_run`, `diagnostics_parse`.

**Formatting:** 4-space indent, no tabs, 100-col max, always brace all control blocks, `static` on all private helpers, early return for errors.

**Commits:** `type(scope): short summary` (types: `feat`/`fix`/`test`/`style`/`docs`/`refactor`).

## Tests

Golden test fixtures live in `tests/fixtures/`:
- Valid: `<name>.cplus` + `<name>.expected.c` (identity output)
- Invalid: `<name>.cplus` only (no `.expected.c`; pipeline must return rc=1)

`_clang`-suffixed test targets are silently skipped if `clang` is not installed — this is intentional.

## Current Specs

- v1 spec: [docs/spec-v1.md](docs/spec-v1.md) — CLI, exit codes, file extensions, identity pipeline
- v2 spec: [docs/spec-v2.md](docs/spec-v2.md) — `weak`/`unique`/`move`/`class` semantics, error codes E201–E206, lowering rules
- Roadmap: [docs/roadmap.md](docs/roadmap.md)

## Pitfalls

- `system(3)` (not `fork`/`exec`) is used to invoke the compiler — stdout+stderr are redirected to a temp file and read back.
- `-D_FORTIFY_SOURCE=3` requires optimization; sanitizers only activate in `Debug` builds.
- v1 identity copy is verbatim (byte-for-byte) — no parsing occurs in v1.
- `.hplus`/`.cplus` are source files; `.h`/`.c` in `examples/` are **generated artefacts — never edit them directly**.
- v2 uses an **island AST**: only cplus-specific syntax nodes exist; all C23 passthrough text is stored verbatim in `NODE_PASSTHROUGH` blobs. Do not attempt to parse standard C23 — it is never parsed.
