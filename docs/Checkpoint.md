# Checkpoint

**Date:** August, 2025  
**Milestone:** Baseline – parser, grammar, and test suite synchronized and passing.

---

## Current State

- **Grammar (`docs/grammar.mpc`)**
  - Updated to remove unsupported regex lookaheads.
  - `c_chunk` rules cover:
    - Preprocessor lines (`#…`)
    - Comments (`//…`, `/* … */`)
    - C declarations (single and multi-line ending in `;`)
    - Block openers (`…{`) and standalone closers (`}`)
  - Param arrays supported in signatures: `int a[10]`
  - Modifiers unified into `mods_opt`: accepts `public static` and `static public`.

- **Generator (`gen_parser/src/main.c`)**
  - Fixed `'/'` handling: single branch with peek for `//`, `/*`, or regex.
  - Emits clean embedded grammar string.
  - Collects rule names correctly and preserves order.

- **Parser (`parser/src/main.c`)**
  - Regenerated from the updated grammar.
  - Successfully builds with `make`.
  - CLI supports:
    - `-G` to print grammar
    - `-f <file>` to parse file
    - `-x "<expr>"` to parse inline text

- **Tests (`tests/`)**
  - All tests `00` through `14`, plus `demo` and `mini`, parse successfully.
  - Problematic case (`13_c_chunk_mixed.cplus.h`) now passes.
  - `do_tests.sh` updated to cover both `.cplus.h` and `.cplus`.

---

## Baseline Marker

- Commit message:  
  `"Baseline: snapshot of current project (grammar, parser, tests all green)"`
- Git tag:  
  `baseline`

This represents the reproducible state where the grammar, generator, and parser are fully synchronized.

---

## Next Steps

1. **Expand Grammar**
   - Allow forward declarations with generics: `class Foo<T>;`
   - Extend `typedef_class` / `typedef_interface` handling.
   - Consider multi-line preprocessor directives with `\`.

2. **Testing**
   - Add edge-case tests:
     - `extern "C" { … }`
     - multi-line macros
     - empty interfaces / empty classes
   - Add negative tests (ensure invalid syntax is rejected).

3. **Tooling**
   - Create a `Makefile` for cplus root to:
     - regenerate parser automatically:
     ```bash
     make regen
     ```
     - integrate test runner (`do_tests.sh`)
     ```bash
     make test
     ```

4. **Documentation**
   - Expand `README.md` with a quickstart:
     - clone
     - build
     - run parser over tests
   - Document baseline process.

---

**Summary:**  
All components are stable. This checkpoint freezes the **baseline** where the grammar, parser, and tests align 100%. Future work can safely build from here.


