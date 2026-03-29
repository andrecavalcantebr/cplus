# cplus

Experimental transpiler to teach both OO and other resources in "modern" languages and how they can be implemented in bare metal using standard C.

## Vision

`cplus` is a language close to C, with incremental extensions to it (ex.: RAII, OO syntax); it is transpiled to C in a form that can be visible and manipulated for humans.

The output of the code is a C code seems like manually make by humans, without any obfurscated code.

## Project reset

This repository was restarted in a new architecture.
The old code (MPC based) was archived in a historic branch/tag.

## Core principles

- Preserve C compatibility first.
- Keep generated C readable.
- Evolve in small, testable steps.
- Validate with GCC first, then Clang.
- Prefer explicit semantics over heuristics.
- `.hplus` / `.cplus` are source; `.h` / `.c` are generated artefacts — never edit generated files.
- When a `.cplus` file includes a `.hplus` header, **transpile headers first**. The lowering pass rewrites `#include "foo.hplus"` → `#include "foo.h"` in the generated `.c`; enforce ordering via Make/CMake.

## Roadmap

- **v1**: `C23 -> C23`
  - Identity transpiler.
  - Parse/validate source and regenerate equivalent C23.
  - No new language features yet.

- **v2**: `cplus (small new statements) -> C23`
  - Introduce minimal syntax extensions with simple lowering.

- **v3**: `cplus (more elaborate statements) -> C23`
  - Advanced statement transformations (ex.: structured RAII/resource patterns).

- **v4**: `cplus (additional small statements) -> C23`
  - Second wave of small syntax improvements and consistency refinements.

- **v5+**: `cplus (OO syntax) -> C23`
  - Classes/objects syntax.
  - Smart pointers and stronger RAII model.
  - Didactic OOP lowering to plain C.

## Build (GCC + CMake)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug   # configure (ASan/UBSan enabled)
cmake --build build                        # build
ctest --test-dir build --output-on-failure # run all tests
cmake --build build --target memcheck      # valgrind (optional)
```

> After adding any new `.c` file, re-run `cmake -B build` — sources are collected with `GLOB_RECURSE CONFIGURE_DEPENDS`.

## Directory layout

- `docs/` documentation
- `src/` project sources (transpiler itself, in C)
- `tests/` automated tests
- `examples/` `.hplus` / `.cplus` source examples and their generated `.h` / `.c` artefacts
- `vendor/` third-party sources
- `inc/` deployed/generated/third-party headers
- `lib/` deployed/generated/third-party libraries
- `build/` temporary build output

## Current status

**v1 — complete.** **v2 — in progress.**

### v1 — done

- Build system (CMake + GCC, `-std=c23`, ASan/UBSan in Debug)
- CLI (`cplus <file.(h|c)plus> [...] [-o output] [--cc gcc|clang] [--std c23] [-T]`)
- `-T` / `--dump-tokens`: print the token stream to stdout (or `-o` file), analogous to `gcc -E`
- Pipeline: syntax validation via `gcc -fsyntax-only`, identity copy on success
- GCC compatibility shim: remaps `-std=c23` → `-std=c2x` on GCC < 14
- Diagnostics parser: structured `DiagnosticList` with file/line/column/severity/message/context
- `StrBuf` ADT (`src/strbuf.{h,c}`): growable byte buffer used by diagnostics and lowering
- Golden tests: valid C23 input (identity output) and invalid C23 input (rc=1)
- All sources follow coding-style conventions (file headers, include guards)
- Paired development: Andre Cavalcante + Claude Sonnet 4.6

### v2 — in progress

v2 introduces `weak`, `unique`, `move`, and `class` as first-class cplus constructs
transpiled to idiomatic C23. See [docs/spec-v2.md](docs/spec-v2.md) for full semantics
and lowering rules.

**Done:**
- v2 test fixtures created (`tests/fixtures/v2_*.cplus` + `.expected.c`)
- `StrBuf` ADT (foundation for the lowering output buffer)
- Lexer (`src/lexer.{h,c}`) — island scanner, keyword table, pull-based `lexer_next`/`lexer_peek`

**In progress:**
- Island AST (`src/ast.{h,c}`) — sparse tree: cplus nodes + `NODE_PASSTHROUGH` for C23 verbatim
- Parser (`src/parser.{h,c}`) — recursive descent, produces AST; detects E201–E202/E204–E206
- Lowering (`src/lowering.{h,c}`) — AST visitor, emits C23 via `StrBuf`; rewrites `#include "*.hplus"` → `#include "*.h"` in output
