# Cplus – Checkpoint

Date: September, 2025

Current focus: C‑first pipeline that desugars `class` into strict, readable C and leaves the rest of the input untouched.

What’s implemented
- Grammar (docs/grammar.mpc)
  - C‑like grammar with `class` integrated as a type (similar to `struct`).
  - Class body supports fields and method prototypes; each member may be prefixed by `public` or `private` (at most one).
  - Permissive function parameter parsing (balanced parentheses) so `int *p`, `Foo *self` etc. work.
  - C‑style trailing declarators after a class definition: `class Foo { ... } *p, v;`.
  - Typedefs: `typedef int T;`, use of typedef names in prototypes and params, and prototypes like `I f(I *p);` handled explicitly.
  - Forward declaration: `class Foo;`.

- Transformer (parser/src/ast_transf.c)
  - Emits:
    - `typedef struct Foo Foo;`
    - `struct Foo { /* fields only */ };`
    - Free function prototypes named `Foo_method(...)` (no implicit self injection).
  - Strips `public`/`private` from output (semantic checks deferred to Cplus).
  - Echoes non‑class C code unchanged (preprocessor, declarations, function definitions).

Examples (-x)
- `class Foo{ private int x; int f(int n); } foo;`
  →
  ```c
  typedef struct Foo Foo;
  struct Foo {
      int x;
  };
  struct Foo foo;
  int Foo_f(int n);
  ```
- `class Bar{ int k; void g(void); } *p, v;`
  →
  ```c
  typedef struct Bar Bar;
  struct Bar {
      int k;
  };
  struct Bar *p, v;
  void Bar_g(void);
  ```

Out of scope for this checkpoint
- Interfaces, inheritance, vtables/RTTI, and access‑rule enforcement (parsing only for `public|private`).

Next steps
- Add `interface` with a similar, explicit mapping to C and disallow instantiation except via pointer.
- Optional: enforce access rules in a dedicated semantic pass (not in the C output stage).
- Grow the test corpus around pointer parameters, typedef combinations, and error cases.

Notes
- The output is intended to be compiled by any C23 compiler.
- Generated identifiers and comments are in English to keep the material universal for teaching.
