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

## Build (GCC + CMake generating Makefiles)

```bash
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

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

v1 in progress — pipeline and diagnostics infrastructure complete.

### What is done

- Build system (CMake + GCC, `-std=c23`, ASan/UBSan in Debug)
- CLI (`cplus <file.(h|c)plus> [...] [-o output] [--cc gcc|clang] [--std c23]`)
- Pipeline: syntax validation via `gcc -fsyntax-only`, identity copy on success
- GCC compatibility shim: remaps `-std=c23` → `-std=c2x` on GCC < 14
- Diagnostics parser: structured `Diagnostic` model with file/line/column/severity/message/context (caret lines)
- Golden tests: valid C23 input (identity output) and invalid C23 input (no output, rc=1)
- All source and header files follow the coding-style conventions (file headers, include guards)
- Paired development: Andre Cavalcante + Claude Sonnet 4.6

### What is pending (still in v1)

- Clang validation path tested in CI
- Architecture doc updated for diagnostics module
- `examples/` golden fixtures expanded
