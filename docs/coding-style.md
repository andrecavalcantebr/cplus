# cplus Coding Style

This document defines the coding conventions for the `cplus` project.

## 1. General principles

- Keep code simple and readable.
- Prefer explicit behavior over implicit behavior.
- Write comments only when they add real context.
- Keep generated C code human-readable.
- Follow C23 standard compatibility.

## 2. File header template

Every `.c` and `.h` file must start with this header pattern.

### `.c` files

```c
/*
 * FILE: filename.c
 * DESC.: this file is the implementation of ...
 * AUTHOR: Andre Cavalcante
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */
```

### `.h` files

```c
/*
 * FILE: filename.h
 * DESC.: this file is the declaration of ...
 * AUTHOR: Andre Cavalcante
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */
```

## 3. Header guards

Use uppercase guards without leading double underscore.

- ✅ `CPLUS_VALIDATOR_H`
- ❌ `__CPLUS_VALIDATOR_H`

Template:

```c
#ifndef CPLUS_NAME_H
#define CPLUS_NAME_H

// includes
// declarations

#endif // CPLUS_NAME_H
```

## 4. File organization

### `.c` file section order

1. File header comment
2. Includes
3. Internal data structures (`typedef`, `struct`, `enum`)
4. Internal static function declarations
5. Public function implementations
6. Internal static function implementations

### `.h` file section order

1. File header comment
2. Include guard
3. Includes
4. Public data structures
5. Public function prototypes
6. End guard

## 5. Naming conventions

- Files: `snake_case.c`, `snake_case.h`
- Functions: `snake_case`
- Variables: `snake_case`
- Struct types: `PascalCase` or `snake_case_t` (pick one and stay consistent per module)
- Macros/constants: `UPPER_SNAKE_CASE`
- Prefix public symbols by module when possible:
  - `validator_check_syntax`
  - `pipeline_run`

## 6. Includes policy

- In `.c`, include its own header first.
- Then C standard headers.
- Then project headers.
- Then third-party headers.

Example:

```c
#include "validator.h"

#include <stdio.h>
#include <stdlib.h>

#include "diagnostics.h"
#include "file_io.h"
```

## 7. Functions

- Keep functions short and focused.
- Use early return for error handling.
- Avoid deep nesting.
- Validate input pointers when relevant.
- Mark private helpers as `static`.

## 8. Comments

- Write comments in English.
- Prefer meaningful names over excessive comments.
- Use comments for:
  - non-obvious behavior
  - invariants
  - safety constraints
  - transformation rationale in transpiler stages

## 9. Formatting

- Indentation: 4 spaces (no tabs).
- Max line length: 100 columns (recommended).
- One statement per line.
- Always use braces for control blocks, even for single-line statements.

## 10. Compiler warnings and quality

Code must build cleanly with:

- `-Wall`
- `-Wextra`
- `-Wpedantic`

Treat warnings as errors in CI when possible.

## 11. `main.c` baseline template

```c
/*
 * FILE: main.c
 * DESC.: entry point of the system
 * AUTHOR: Andre Cavalcante
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#include <stdio.h>

// auxiliary functions

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("Cplus Transpiler v0.1.0\n");
    return 0;
}
```

## 12. Directory conventions

Current preferred layout:

- `docs/` project documentation
- `src/` source code
- `tests/` test code
- `vendor/` third-party source code
- `build/` generated build files (temporary)
- `inc/` deployed/generated headers (optional workflow)
- `lib/` deployed/generated libraries (optional workflow)

## 13. Git conventions

- Keep commits small and meaningful.
- Commit message style:

```text
type(scope): short summary
```

Examples:

- `feat(pipeline): add syntax validation step`
- `fix(validator): handle clang non-zero exit parsing`
- `docs(style): define header guard convention`
