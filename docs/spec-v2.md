# cplus v2 Specification

## Overview

v2 introduces the first cplus language extensions. All v1 behavior is preserved
unchanged. New constructs are resolved entirely at transpile time — no runtime
library is required in v2.

The generated C targets GCC and Clang. v2 uses `__attribute__((cleanup))`, a
GCC/Clang extension, as the lowering target for `unique`. This is explicitly
accepted because GCC and Clang are the declared target compilers of cplus.

---

## Reserved keywords (v2 additions)

The following identifiers are reserved by cplus and may not be used as variable,
function, type, or parameter names in `.cplus` / `.hplus` source files:

```
weak    unique    move    class
```

Using a reserved keyword as an identifier is a transpile-time error.

These keywords are added to the C23 reserved set — they are not contextual.
The generated C never contains these tokens as identifiers.

---

## Feature 1 — `weak` pointer annotation

### Syntax

```cplus
weak T *name;
weak T *name = expr;
```

### Semantics

`weak` declares a plain non-owning pointer. The programmer asserts that this
pointer does not own the pointed-to resource. No cleanup is generated.

### Lowering

Identity — the `weak` qualifier is stripped:

```c
T *name;
T *name = expr;
```

### Rules

- Any pointer type is allowed.
- `weak` may appear in any scope (file, function, block).
- No restriction on use, assignment, or passing as argument.

---

## Feature 2 — `unique` owning pointer

### Syntax

```cplus
unique(destructor) T *name = expr;
unique(destructor) T *name2 = move(name);
```

`destructor` is the name of a C function that will be called when `name` goes
out of scope.

### Semantics

`unique` declares an owning pointer. Exactly one `unique` variable owns the
resource at any point in time. The destructor is called automatically when the
variable goes out of scope, including on early `return`, `break`, `continue`,
and `goto`.

### Lowering

The transpiler emits a `static inline` cleanup wrapper **immediately before the
function definition** that contains the `unique` declaration (or at file scope
header if the `unique` is at file scope). The wrapper is emitted once per
distinct `destructor` name per translation unit.

```cplus
/* cplus source */
unique(person_destroy) Person *p = person_new("Alice", 30);
```

```c
/* generated C */

/* cleanup wrapper — emitted once per destructor, before the function */
static inline void _cplus_cleanup_person_destroy(Person **pp) {
    if (*pp != NULL) { person_destroy(*pp); *pp = NULL; }
}

/* declaration with cleanup attribute */
Person *p __attribute__((cleanup(_cplus_cleanup_person_destroy)))
          = person_new("Alice", 30);
```

The `*pp = NULL` in the wrapper prevents double-free if the pointer is reused
after cleanup in non-cplus code.

### Builtin wrappers for common C functions

The transpiler ships `inc/cplus_cleanup.h` with pre-generated wrappers for
common C standard and POSIX destructors:

| cplus `unique(fn)` | generated wrapper |
|--------------------|-------------------|
| `unique(fclose)` | `_cplus_cleanup_fclose` |
| `unique(free)` | `_cplus_cleanup_free` |
| `unique(close)` | `_cplus_cleanup_close` |

The programmer never writes these wrappers. The transpiler includes
`cplus_cleanup.h` automatically when any builtin destructor is referenced.

### Rules

- `unique` may appear in any scope (function, block, loop body).
- The declaration must include an initializer: `unique(fn) T *p = expr;`
- Uninitialized `unique` declaration is a transpile-time error.
- Passing a `unique` pointer as a function argument is allowed (non-owning use).
- Assigning a `unique` pointer to a plain pointer is a transpile-time error
  (see: copy prohibition below).

---

## Feature 3 — `move` transfer of ownership

### Syntax

```cplus
unique(destructor) T *name2 = move(name);
```

`move` is a transpiler construct, not a macro or function. It is only valid
on the right-hand side of a `unique` declaration.

### Semantics

Transfers ownership from `name` to `name2`. After `move`, `name` is in the
*moved* state: it holds `NULL` and must not be used until reassigned.

### Lowering

```cplus
unique(person_destroy) Person *q = move(p);
```

```c
Person *q __attribute__((cleanup(_cplus_cleanup_person_destroy))) = p;
p = NULL;
```

### Use-after-move detection

The transpiler tracks moved variables in its symbol table. For each statement
that references a variable in the *moved* state, the transpiler injects an
assert **before the statement**:

```c
assert(p != NULL && "use-after-move: 'p'");
```

When `NDEBUG` is defined, the assert is suppressed. The resulting behavior is
undefined — equivalent to dereferencing `NULL` in C. This is documented and
intentional (same contract as standard `assert`).

**Reassignment resets the moved state:**

```cplus
unique(person_destroy) Person *p = person_new("Alice", 30);
unique(person_destroy) Person *q = move(p);   /* p is moved */
p = person_new("Bob", 25);                    /* p is live again */
person_print(p);                              /* OK — no assert */
```

### Rules

- `move(x)` is only valid as the initializer of a `unique` declaration.
- Using `move(x)` in any other expression position is a transpile-time error.
- After `move(x)`, `x` is in the *moved* state until reassigned.
- Reassignment of a *moved* variable is allowed and resets its state to *live*.

---

## Feature 4 — copy prohibition for `unique`

Assigning a `unique` pointer to a non-`unique` pointer (implicit copy) is a
transpile-time error:

```cplus
unique(person_destroy) Person *p = person_new("Alice", 30);
Person *q = p;   /* ERROR: cannot copy a unique pointer */
fn(p);           /* OK   : passing as argument is a non-owning use */
```

### Rules

- Any assignment `T *lhs = unique_rhs` where `lhs` is not declared `unique` is
  a transpile-time error.
- Passing a `unique` pointer as a function argument (`fn(p)`) is allowed.
- Passing the address of a `unique` pointer (`fn(&p)`) is allowed and is the
  conventional way to transfer ownership through a function parameter.

---

## Feature 5 — `class` declaration

### Syntax

```cplus
/* in .hplus */
class Name {
    T field1;
    T field2;
    /* ... */
};
```

### Semantics

`class` is the cplus spelling of a named aggregate type. In v2 all fields are
implicitly public. Access modifiers (`public`, `private`) are reserved for v3.

The type `Name` is usable directly (no `struct Name` prefix) in all `.cplus`
and `.hplus` files — identical to the C++ convention.

### Lowering

```cplus
class Person {
    char name[64];
    int  age;
};
```

```c
/* generated in the .h file */
typedef struct Person Person;
struct Person {
    char name[64];
    int  age;
};
```

### Rules

- `class` is only valid at file scope (top level of `.hplus` or `.cplus`).
- Nested `class` declarations are a transpile-time error in v2.
- Fields may be any C23 type, including pointers and arrays.
- Methods are not supported in v2 — they are scoped to v3+.
- Access modifiers (`public`, `private`) are reserved keywords but produce a
  transpile-time error if used in v2.
- The generated `typedef` makes `Name` available to C code that includes the
  generated `.h` — no `struct` prefix needed in the C world either.

---

## Transpiler pipeline (v2)

```
.cplus / .hplus source
        │
        ▼
   Lexer (v2 keywords added)
        │
        ▼
   Symbol table builder
   (tracks: unique vars, moved state, class names)
        │
        ▼
   Lowering pass
   ├── class  → typedef struct
   ├── weak   → strip qualifier
   ├── unique → emit cleanup wrapper + __attribute__((cleanup(...)))
   └── move   → split into assignment + NULL; inject use-after-move asserts
        │
        ▼
   C23 output (.c / .h)
        │
        ▼
   Syntax validation (gcc/clang -fsyntax-only)  ← unchanged from v1
```

---

## Example — full `Person` module in v2

### `person.hplus`

```cplus
#ifndef PERSON_H
#define PERSON_H

class Person {
    char name[64];
    int  age;
};

Person *person_new(const char *name, int age);
void    person_destroy(Person **p);
void    person_print(const Person *p);

#endif
```

### `person.cplus`

```cplus
#include "person.hplus"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Person *person_new(const char *name, int age) {
    Person *p = malloc(sizeof(Person));
    if (p == NULL) return NULL;
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';
    p->age = age;
    return p;
}

void person_destroy(Person **p) {
    if (*p == NULL) return;
    free(*p);
    *p = NULL;
}

void person_print(const Person *p) {
    printf("Person { name: \"%s\", age: %d }\n", p->name, p->age);
}
```

### `main.cplus`

```cplus
#include "person.hplus"

int main(void) {
    unique(person_destroy) Person *alice = person_new("Alice", 30);
    unique(person_destroy) Person *bob   = move(alice);

    person_print(bob);

    /* alice is in moved state — use would inject assert */
    /* bob is destroyed automatically at end of scope    */
    return 0;
}
```

### Generated `person.h`

```c
#ifndef PERSON_H
#define PERSON_H

typedef struct Person Person;
struct Person {
    char name[64];
    int  age;
};

Person *person_new(const char *name, int age);
void    person_destroy(Person **p);
void    person_print(const Person *p);

#endif
```

### Generated `main.c`

```c
#include "person.h"
#include <assert.h>

static inline void _cplus_cleanup_person_destroy(Person **pp) {
    if (*pp != NULL) { person_destroy(pp); *pp = NULL; }
}

int main(void) {
    Person *alice __attribute__((cleanup(_cplus_cleanup_person_destroy)))
                  = person_new("Alice", 30);
    Person *bob   __attribute__((cleanup(_cplus_cleanup_person_destroy)))
                  = alice;
    alice = NULL;   /* move */

    person_print(bob);

    return 0;
}
```

---

## Error catalogue (v2)

| Code | Message | Trigger |
|------|---------|---------|
| `E201` | `reserved keyword used as identifier: 'X'` | `int weak = 0;` |
| `E202` | `unique declaration requires an initializer` | `unique(fn) T *p;` |
| `E203` | `cannot copy a unique pointer: 'X'` | `T *q = unique_p;` |
| `E204` | `move() is only valid as a unique initializer` | `fn(move(p))` |
| `E205` | `nested class declaration is not supported` | `class A { class B {...}; };` |
| `E206` | `access modifiers not supported in v2` | `class A { private: int x; };` |
| `E207` | `class declaration is only valid at file scope` | `class` inside function |

---

## Backward compatibility

All valid v1 `.cplus` / `.hplus` sources are valid v2 sources, provided they do
not use any of the new reserved keywords (`weak`, `unique`, `move`, `class`) as
identifiers.

---

## Future versions (informative)

| Version | Planned additions |
|---------|------------------|
| v3 | `class` with methods; `public` / `private` fields (transpiler-enforced, flat struct — Strategy B); constructors and destructors as named methods |
| v4 | `shared(fn) T *ptr` — reference-counted owning pointer with runtime support |
| v5+ | OO syntax: inheritance by composition, virtual dispatch |
