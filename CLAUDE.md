# cplus

Experimental C transpiler: `.cplus`/`.hplus` source → C23 output.
Active branch: `feat/v2`. `main` is frozen at v1 DoD.

## Build and Test

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug   # configure (ASan/UBSan enabled)
cmake --build build                        # build
ctest --test-dir build --output-on-failure # run all tests
cmake --build build --target memcheck      # valgrind
```

> After adding any new `.c` file, reconfigure first (`cmake -B build`) — sources use `GLOB_RECURSE CONFIGURE_DEPENDS`.

## Architecture

| Module | Files | Role |
|--------|-------|------|
| driver | `src/main.c` | CLI, builds `PipelineOptions`, calls `pipeline_run()` |
| pipeline | `src/pipeline.{h,c}` | load → validate → identity copy (v1) / lower (v2) |
| compiler_validator | `src/compiler_validator.{h,c}` | `gcc`/`clang -fsyntax-only` via `system(3)`; GCC <14 shim: `-std=c23→-std=c2x` |
| diagnostics | `src/diagnostics.{h,c}` | `DiagnosticList`; caller must call `diagnostics_free_list()` |
| strbuf | `src/strbuf.{h,c}` | Growable byte buffer ADT (`append_bytes`, `append_cstr`, `take`) |

v2 adds: `src/lexer.{h,c}` (**done**), `src/ast.{h,c}`, `src/parser.{h,c}`, `src/lowering.{h,c}` — see [docs/architecture.md](docs/architecture.md).

`cplus_core` static lib = all `src/*.c` except `main.c`. Tests link against it. `CPLUS_FIXTURES_DIR` is a CMake compile-time define — never build by hand.

## Code Style

**Language:** C23 strict (`-std=c23`, `CMAKE_C_EXTENSIONS OFF`). `strdup`/`strndup` are C23 — no `_POSIX_C_SOURCE` in src/lib code. Test files needing `unlink` place `#define _POSIX_C_SOURCE 200809L` **before** the file header comment.

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

**Include order:** own `.h` → stdlib → project → third-party.

**Naming:** `snake_case` functions/variables, `PascalCase` structs, `UPPER_SNAKE_CASE` macros. Public symbols prefixed by module: `pipeline_run`, `diagnostics_parse`.

**Formatting:** 4-space indent, no tabs, 100-col max, always brace all control blocks, `static` on all private helpers, early return for errors.

**Commits:** `type(scope): short summary` (`feat`/`fix`/`test`/`style`/`docs`/`refactor`).

See [docs/coding-style.md](docs/coding-style.md) and [docs/build-flags.md](docs/build-flags.md) for full details.

## Tests

Golden fixtures in `tests/fixtures/`:
- Valid: `<name>.cplus` + `<name>.expected.c`
- Invalid: `<name>.cplus` only — pipeline must return rc=1

`_clang`-suffixed targets are silently skipped if `clang` is absent — intentional.

## Current Specs

- v1: [docs/spec-v1.md](docs/spec-v1.md) — CLI, exit codes, identity pipeline
- v2: [docs/spec-v2.md](docs/spec-v2.md) — `weak`/`unique`/`move`/`class`, error codes E201–E206
- Roadmap: [docs/roadmap.md](docs/roadmap.md)

## Pitfalls

- `system(3)` (not `fork`/`exec`) invokes the compiler; output redirected to a temp file.
- `-D_FORTIFY_SOURCE=3` requires optimization; sanitizers only active in `Debug`.
- v1 identity copy is verbatim — no parsing.
- `.hplus`/`.cplus` are source; `.h`/`.c` in `examples/` are generated — never edit them.
- The lowering pass rewrites `#include "*.hplus"` → `#include "*.h"` in generated output. Always transpile `.hplus` before `.cplus` — the C compiler will not find `foo.hplus` when compiling the generated `.c`. Enforce order via Make/CMake.
- `-T`/`--dump-tokens` prints the token stream; respects `-o`; skips transpile and validate stages.
