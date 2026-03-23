# cplus v1 Architecture

## Scope

Version 1 is an identity transpiler for C23:

- Input: `.cplus` / `.hplus` source files (valid C23 in v1)
- Output: Equivalent `.c` / `.h` files (do not edit)
- No language extensions in this version

## File extension contract

```
programmer writes:          transpiler emits:
  foo.hplus  ──────────────►  foo.h   (C header, consumed by C world)
  foo.cplus  ──────────────►  foo.c   (C source)
```

External C `.h` headers (from system or third-party libraries) flow through
unchanged and are never transpiled. In `.cplus` / `.hplus` sources:

- Include cplus-owned interfaces as `#include "foo.hplus"`
- Include external C headers as `#include <system.h>` or `#include "vendor.h"`

The generated `.c` / `.h` files replace the `.hplus` includes with the
generated `.h` equivalents, so the C world only ever sees standard `.h` files.

## Design goals

- Keep the pipeline simple and testable
- Use existing compilers as syntax oracles
- Produce human-readable output
- Keep extension points for future cplus features

## Pipeline

1. Load source file
2. Run syntax validation with:
   - GCC: `gcc -std=c23 -fsyntax-only`
   - Clang: `clang -std=c23 -fsyntax-only`
3. Normalize diagnostics
4. If valid, emit identity output
5. Return non-zero exit code on errors

## Modules

- `driver`: CLI parsing and orchestration
- `io`: file read/write utilities
- `validator`: compiler invocation and status parsing
- `diagnostics`: normalized error model and printing
- `pipeline`: high-level workflow API

## Non-goals (v1)

- Full custom C parser
- Formatting preservation beyond identity copy
- Any cplus extension syntax beyond the file extension contract

## Extension path (v2+)

- Add pre-lowering stage for cplus constructs
- Keep post-lowering validation with GCC/Clang
- Add source mapping for transformed diagnostics
- `.hplus` will carry OO declarations (classes, visibility) — the generated `.h`
  will expose only opaque types and function prototypes to the C world
