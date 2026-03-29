# cplus Architecture

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
This rewrite is the responsibility of the **lowering pass** — a textual
substitution applied to `TK_PASSTHROUGH` blobs that contain `#include "*.hplus"`
directives, not a semantic analysis step.

**Build ordering:** because `foo.cplus` imports `foo.hplus`, the header must be
transpiled before the implementation. This is a build-system concern — use
Make dependency rules or CMake `add_custom_command` to enforce it. cplus does
not follow `#include` chains; type correctness of the generated C is validated
by the downstream `gcc`/`clang -fsyntax-only` stage.

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
  `StrBuf` (see below) that accumulates lines and is transferred to the
  `Diagnostic` via `strbuf_take()`.
- Owned strings are built with `strndup` / `strdup`, which are standard C23
  (ISO/IEC 9899:2024 §7.27.6) — no `_POSIX_C_SOURCE` feature-test macro is
  required.

**Memory contract:**

- `diagnostics_parse()` returns a fully-owned list; caller must call
  `diagnostics_free_list()`.
- `diagnostics_free_list()` frees every `file`, `message`, and `context` string,
  then the `items` array.

### `io` (inline in pipeline.c)

File read/write utilities (`read_entire_file`, `write_entire_file`) embedded as
`static` helpers in `pipeline.c`. No other module reads or writes files directly.
Will be extracted to a dedicated module if a second consumer appears (e.g. source
maps in v3+).

### `strbuf` (src/strbuf.c)

Minimal growable byte-buffer ADT used wherever output is accumulated
incrementally before being written or transferred.

**API:**

```c
typedef struct { char *data; size_t len; size_t cap; } StrBuf;

void   strbuf_init(StrBuf *b);
void   strbuf_free(StrBuf *b);
int    strbuf_append_bytes(StrBuf *b, const char *data, size_t n); /* 1=ok, 0=OOM */
int    strbuf_append_cstr(StrBuf *b, const char *s);
char  *strbuf_take(StrBuf *b, size_t *out_len); /* transfers ownership; resets b */
```

**Design:**
- Doubles capacity on growth (minimum 256 bytes).
- `data[len]` is always `'\0'` — safe to pass to C string APIs.
- `strbuf_take()` transfers the owned buffer to the caller and resets the
  `StrBuf` to zero, making single-allocation patterns cheap and explicit.

**Consumers:**
- `diagnostics.c` — accumulates `context` lines before assigning to `Diagnostic.context`.
- `lowering.c` (v2, in progress) — accumulates the entire C23 output for a translation unit.

## Non-goals (v1)

- Full custom C parser
- Formatting preservation beyond identity copy
- Any cplus extension syntax beyond the file extension contract

## Extension path (v2+)

v2 introduces an island AST and a lowering pass between parsing and output.
The pipeline expands from:

```
load → validate → identity copy
```

to:

```
load → lex → parse (island AST) → lower (C23 via StrBuf) → validate → write
```

New modules planned for v2:

| Module | Files | Role |
|--------|-------|------|
| lexer | `src/lexer.{h,c}` | **Implemented.** Island scanner; keyword table; pull-based `lexer_next`/`lexer_peek`; `-T`/`--dump-tokens` flag |
| ast | `src/ast.{h,c}` | Sparse island AST: cplus nodes + `NODE_PASSTHROUGH` for verbatim C23 |
| parser | `src/parser.{h,c}` | Recursive descent; produces AST; detects E201/E202/E204/E205/E206 |
| lowering | `src/lowering.{h,c}` | AST visitor; emits C23 output via `StrBuf`; detects E203; rewrites `#include "*.hplus"` → `#include "*.h"` |

### Island parser philosophy

The cplus parser tracks only what is necessary to enforce ownership rules
(E201–E206). It does **not** resolve types across files, follow `#include`
chains, or maintain a cross-translation-unit symbol table. Specifically:

- `unique Person *p = ...` — `Person` is opaque; cplus only sees `TK_IDENT`
- Type correctness (undefined `Person`, type mismatches) is delegated to the
  downstream compiler
- Only names of `unique`-declared variables in the **current file** are tracked
  (for E203 copy-prohibition)

This keeps the parser simple and avoids reimplementing what gcc/clang already do well.

See [docs/spec-v2.md](spec-v2.md) for the full language specification, lowering
rules, and error catalogue.
