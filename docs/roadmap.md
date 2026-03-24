# cplus Roadmap

## Purpose

This roadmap defines incremental versions of `cplus` and the minimum quality bar for each stage.

A feature/version is only complete when all **Definition of Done (DoD)** items are satisfied.

---

## Global Definition of Done (applies to all versions)

- [ ] Code builds on Linux with GCC.
- [ ] CI pipeline is green.
- [ ] Unit and golden tests pass.
- [ ] No critical warnings in CI (`-Wall -Wextra -Wpedantic`).
- [ ] Documentation is updated (`README` + `docs/spec.md` when applicable).
- [ ] At least one example is added/updated in `examples/` (or tests fixture).
- [ ] Generated C code remains human-readable.

---

## v1 — `C23 -> C23` (Identity Transpiler)

### Scope

- Establish the `.cplus` / `.hplus` source file convention.
- Parse and validate C23 input.
- Regenerate equivalent C23 output (`.c` / `.h` artefacts).
- No language extension yet — v1 sources are valid C23.

### DoD

- [x] CLI accepts input file and emits output file.
- [x] Syntax errors are reported with file/line/column.
- [x] Golden tests cover valid and invalid C23 samples.
- [x] Output compiles with GCC.
- [ ] Output compiles with Clang.
- [ ] Basic architecture documented (parser/AST/lowering/diagnostics).

---

## v2 — `cplus (small new statements) -> C23`

### Scope

- Introduce a first minimal extension set.
- Keep transformation rules explicit and simple.

### DoD

- [ ] Grammar for each new statement is documented.
- [ ] Semantic rules are documented with examples.
- [ ] Lowering rules are deterministic (no hidden heuristics).
- [ ] Negative tests for malformed syntax and invalid semantics.
- [ ] Generated C for new statements is readable and compiles on GCC/Clang.

---

## v3 — `cplus (more elaborate statements) -> C23`

### Scope

- Add structured advanced statements (e.g., RAII/resource patterns).
- Handle nested constructs safely.

### DoD

- [ ] Control-flow interactions are specified (`break`, `continue`, `return`, `goto`).
- [ ] Nested lowering is validated with dedicated tests.
- [ ] Error messages explain invalid control flow clearly.
- [ ] Resource cleanup ordering is deterministic and tested.
- [ ] Regression suite prevents breakage of v1/v2 behavior.

---

## v4 — `cplus (additional small statements) -> C23`

### Scope

- Add second wave of lightweight syntax features.
- Improve consistency and ergonomics.

### DoD

- [ ] New features do not introduce ambiguity with existing grammar.
- [ ] Existing code remains backward compatible (or migration is documented).
- [ ] Tests include mixed usage with previous versions' features.
- [ ] Performance baseline is measured against v3 and documented.

---

## v5+ — `cplus (OO syntax) -> C23`

### Scope

- Introduce object-oriented syntax (classes and related features).
- Keep generated C educational and explicit.

### DoD

- [ ] OO syntax and semantics are formally specified.
- [ ] Lowering model to C structs/functions is documented.
- [ ] Lifecycle model (RAII/smart pointers) is defined and tested.
- [ ] At least one non-trivial end-to-end example compiles and runs on GCC/Clang.
- [ ] Documentation includes “under-the-hood” explanation for teaching use.

---

## Backlog candidates (post-v1)

- [ ] `resource(init; success; cleanup; error) statement`
- [ ] Better diagnostics and source mapping
- [ ] Formatting preservation strategy
- [ ] Cross-file transpilation support
- [ ] Standard library helper layer for generated patterns

---

## Release policy

- Patch: bug fixes, no syntax/semantic changes.
- Minor: new statements/features, backward compatible.
- Major: syntax or semantic breaking changes.

---

## Exit criteria for each milestone

A milestone can be closed only if:

1. All scoped features are implemented.
2. All DoD items are checked.
3. CI is stable for at least one full merge cycle.
4. Version notes are published in `docs/changelog.md` (or `CHANGELOG.md`).
