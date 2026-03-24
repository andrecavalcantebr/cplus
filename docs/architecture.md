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

### `driver` (src/main.c)

CLI parsing and orchestration. Reads `argc/argv`, builds a `PipelineOptions`
struct, calls `pipeline_run()`, and maps the return code to process exit status.

### `pipeline` (src/pipeline.c)

High-level workflow: loads the source file, delegates to `compiler_validator`,
and on success copies the input verbatim to the output path (identity transform).
Returns 0 on success, 1 on validation failure, -1 on I/O error.

### `compiler_validator` (src/compiler_validator.c)

Invokes `gcc` or `clang` with `-x c -std=<std> -fsyntax-only` via `system(3)`,
captures combined stdout/stderr to a temp file, and returns the raw output plus
the compiler exit status. Contains a GCC version shim: `gcc -std=c23` is
rewritten to `gcc -std=c2x` for GCC < 14 (detected at runtime via
`gcc -dumpversion`).

### `diagnostics` (src/diagnostics.c)

Parses raw compiler output (GCC or Clang) into a structured `DiagnosticList`.

**Data model:**

```c
typedef enum { DIAG_ERROR, DIAG_WARNING, DIAG_NOTE } DiagnosticSeverity;

typedef struct {
    char*              file;      /* owned, heap-allocated */
    int                line;
    int                column;
    DiagnosticSeverity severity;
    char*              message;   /* owned */
    char*              context;   /* caret + source snippet, or NULL */
} Diagnostic;

typedef struct {
    Diagnostic* items;
    size_t      count;
    size_t      capacity;
} DiagnosticList;
```

**Parser design:**

- Walks the raw output by pointer (`const char *p`) without fixed buffers or
  intermediate copies; no `strtok`/`strtok_r` is used.
- Each line is delimited as `[line_start, line_end)` — NUL is never assumed at
  `line_end`.
- `try_parse_primary()` matches lines of the form
  `<file>:<line>:<col>: <severity>: <message>` using only pointer arithmetic
  and `strtol`.
- Lines that do not match the primary pattern are collected into the `context`
  field of the preceding `Diagnostic` (caret markers, source snippets) via a
  dynamic `append_line()` helper that doubles its buffer as needed.
- Owned strings are built with `strndup` / `strdup`, which are standard C23
  (ISO/IEC 9899:2024 §7.27.6) — no `_POSIX_C_SOURCE` feature-test macro is
  required.

**Memory contract:**

- `diagnostics_parse()` returns a fully-owned list; caller must call
  `diagnostics_free_list()`.
- `diagnostics_free_list()` frees every `file`, `message`, and `context` string,
  then the `items` array.

### `io` (inline in pipeline.c, v1)

File read/write utilities embedded in the pipeline for v1. Will be extracted to
a dedicated module in v2 when source mapping requires richer I/O.

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
