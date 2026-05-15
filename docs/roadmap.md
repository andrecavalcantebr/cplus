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
- [x] Output compiles with Clang.
- [x] Basic architecture documented (parser/AST/lowering/diagnostics).

---

## v2 — `cplus (small new statements) -> C23`

### Scope

- `struct` auto-typedef at file scope: emit `typedef struct Name Name;` automatically.
- `weak` pointer annotation (non-owning, identity lowering).
- `unique` owning pointer with automatic cleanup via `__attribute__((cleanup))`.
- `move()` ownership transfer with use-after-move detection.
- New reserved keywords: `weak`, `unique`, `move`.
- Keep transformation rules explicit and simple.

### DoD

- [ ] Grammar for each new statement is documented.
- [ ] Semantic rules are documented with examples.
- [ ] Lowering rules are deterministic (no hidden heuristics).
- [ ] Negative tests for malformed syntax and invalid semantics.
- [ ] Generated C for new statements is readable and compiles on GCC/Clang.

---

## v3 — `cplus (ADT method syntax) -> C23`

### Scope

- Function prototypes inside `struct` bodies: extracted at transpile time, not treated as fields.
- `Type.fn()` definition syntax → lowered to `Type_fn()` in output.
- `static` prototype or definition → `static` in generated C (private function).
- Convention: public functions prefixed `TypeName_`; private functions `static typename_`.

### DoD

- [ ] Grammar for prototype-in-struct is documented.
- [ ] `Type.fn()` definition syntax is documented with examples.
- [ ] `static` visibility lowering is specified and tested.
- [ ] Negative tests for malformed syntax (prototype with body inside struct, etc.).
- [ ] Generated C for new statements is readable and compiles on GCC/Clang.
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

## v5+ — `cplus (dot-call syntax) -> C23`

### Scope

- Dot-call syntax: `obj.method(args)` → `TypeName_method(&obj, args)`.
- Requires type inference: transpiler must resolve the type of `obj` to find the correct prefix.
- Composition-first: no inheritance, no vtables — plain struct embedding.
- Keep generated C educational and explicit.

### DoD

- [ ] Dot-call syntax and type inference rules are formally specified.
- [ ] Lowering model (`obj.method(args)` → `Type_method(&obj, args)`) is documented.
- [ ] Composition model (nested structs, delegate calls) is specified and tested.
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
