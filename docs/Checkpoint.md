# Cplus – Checkpoint

## Date: August, 2025

### 🔹 Current Status

#### Specification
- `cplus_specs.md` is consolidated.
- Keywords: `class`, `interface`, `public`, `protected`, `private`.
- Single inheritance + multiple interfaces supported.
- Implementation convention:  
  `type Class_method(Class *self, ...) { ... }`
- Transpilation confirmed to plain C23.
- Open points:
  - Semantics of *generics*.
  - Rules for `typedef` of classes/interfaces.

#### Grammar (`grammar.mpc`)
- Base grammar is functional and stable.
- Main issue: `<c_chunk>` rule
  - Too permissive → invalid tests **wrongly pass** (e.g., failed/2, 6, 7, 8).
  - Too strict → breaks valid `passed/` test files.
- Current trade-off: balancing acceptance of arbitrary C code vs. rejection of invalid Cplus syntax.

#### Tests
- Test structure organized into `tests/passed/` and `tests/failed/`.
- Situation:
  - `passed/` → all OK (with grammar in “safe” mode).
  - `failed/` → cases 2, 6, 7, 8 wrongly accepted when `<c_chunk>` is wide.
- Need to expand **negative test coverage**:
  - Methods declared outside classes.
  - Misplaced access specifiers.
  - Interfaces with fields (should be methods only).
  - Malformed code blocks.

#### Infrastructure
- `gen_parser` stable (C parser generator from grammar).
- `parser/` runs automated tests via `./do_tests.sh`.
- Multi-file integration (`.cp`, `.cp.h`, `.cplus`) not yet supported by automatic `#include` handling.
- Next step: decide whether transpiler should
  - (a) Recursively expand `.cp.h` includes, or
  - (b) Act as an additional pre-processor stage before GCC/Clang.

---

### 🔹 Next Steps

1. **Refine `c_chunk`**
   - Possibly split into `c_chunk_top` (outside classes/interfaces) and `c_chunk_inline` (inside blocks).
   - Goal: reject invalid Cplus while keeping full flexibility for plain C.

2. **Expand negative tests**
   - Add explicit failing examples for encapsulation rules, interface misuse, invalid class members, etc.

3. **Update transpiler**
   - Define preprocessing strategy for multi-file projects.
   - Document build pipeline.

4. **Documentation**
   - Keep `Checkpoint.md` updated at each parser/test iteration.
   - Extend `cplus_specs.md` to cover generics and typedef rules.

---

✅ **Summary**  
The project is stable in its **core features** (classes, interfaces, encapsulation, transpilation to C23).  
The main blocker is the **`<c_chunk>` rule**, which still requires fine-tuning to differentiate between arbitrary C code and invalid Cplus syntax.  

