# cplus Grammar (EBNF)

## Notation

```plaintext
{ x }       zero or more repetitions of x
[ x ]       zero or one occurrence of x (optional)
x | y       ordered alternative (x tried first)
"..."       literal token text
<name>      terminal produced by the lexer
(* ... *)   comment
```

Terminals not enclosed in `"..."` are produced by the island lexer (see §Lexical
grammar below).  Everything that does not match a cplus construct is captured
verbatim in a `<passthrough>` span and copied unchanged to the output.

---

## Lexical grammar

The cplus lexer is an **island scanner**: it classifies only cplus-relevant
tokens; all other source text is returned as `<passthrough>` blobs.

```ebnf
token
    = "weak"                   (* cplus keyword — reserved                 *)
    | "unique"                 (* cplus keyword — reserved                 *)
    | "move"                   (* cplus keyword — reserved                 *)
    | "struct"                 (* C23 keyword — intercepted at file scope  *)
    | "{"                      (* TK_LBRACE                                *)
    | "}"                      (* TK_RBRACE                                *)
    | "("                      (* TK_LPAREN                                *)
    | ")"                      (* TK_RPAREN                                *)
    | ";"                      (* TK_SEMI                                  *)
    | "="                      (* TK_ASSIGN                                *)
    | "*"                      (* TK_STAR                                  *)
    | "&"                      (* TK_AMPERSAND                             *)
    | <ident>                  (* C23 identifier not matching a keyword    *)
    | <passthrough>            (* any other byte sequence; copied verbatim *)
    ;
```

The following source ranges are **always** returned as `<passthrough>`,
regardless of their content, so that cplus keywords embedded inside them are
never misidentified:

- Double-quoted string literals (`"..."`)
- Single-quoted character literals (`'...'`)
- Line comments (`// ...`)
- Block comments (`/* ... */`)
- Preprocessor directives (`#...` at beginning of line, including continuations)

---

## v2 grammar

### Translation unit

```ebnf
translation_unit
    = { top_level_item }
    ;

top_level_item
    = struct_decl              (* brace_depth == 0 (file scope)            *)
    | weak_decl
    | unique_decl
    | <passthrough>            (* verbatim C23 — copied to output          *)
    ;
```

The lexer tracks **brace depth**: `struct_decl` is only recognized when
`brace_depth == 0`.  A `"struct"` token encountered at any other depth is
returned as `<passthrough>`.

---

### Feature 1 — `struct` auto-typedef

```ebnf
struct_decl
    = "struct" <ident> "{" { <passthrough> } "}" ";"
    ;
```

**Lowering:**

```ebnf
struct_decl  →  "typedef struct" <ident> <ident> ";"
                "struct" <ident> "{" { <passthrough> } "}" ";"
```

The field list `{ <passthrough> }` is copied verbatim to the output.  Nested
`struct` bodies (brace_depth > 0) are passthrough; if a nested type also
requires auto-typedef it must be declared separately at file scope, before
the outer struct.

**Error:** none — `struct` at file scope is always valid in cplus v2.

---

### Feature 2 — `weak` pointer annotation

```ebnf
weak_decl
    = "weak" type_ref "*" <ident> [ "=" <passthrough> ] ";"
    ;

type_ref
    = <ident>                  (* single identifier; e.g. "int", "Person"  *)
    ;
```

> **Note:** `type_ref` is a single `<ident>` in the island scanner.
> Multi-token C23 types (`unsigned int`, `long long`, `const char`) must be
> written with a typedef alias when used with `weak` or `unique`.

**Lowering:** identity — the `weak` qualifier is stripped:

```ebnf
weak_decl  →  type_ref "*" <ident> [ "=" <passthrough> ] ";"
```

**Error codes:**

| Code | Trigger |
| ------ | --------- |
| E201 | `weak`, `unique`, or `move` used as an identifier |

---

### Feature 3 — `unique` owning pointer

```ebnf
unique_decl
    = "unique" [ "(" <ident> ")" ] type_ref "*" <ident> "=" init_expr ";"
    ;

init_expr
    = move_expr                (* ownership transfer                       *)
    | <passthrough>            (* arbitrary C23 initializer expression     *)
    ;

move_expr
    = "move" "(" <ident> ")"
    ;
```

The optional `"(" <ident> ")"` names the destructor function.  When omitted,
`free` is used as the default destructor.

**Lowering:**

```ebnf
unique_decl  →  cleanup_wrapper          (* emitted once per destructor   *)
                type_ref "*" <ident>
                "__attribute__((cleanup(" cleanup_fn ")))"
                "=" lowered_init ";"

cleanup_wrapper  →  "static inline void _cplus_cleanup_" <ident>
                    "(" type_ref "**pp) {"
                    "    if (*pp != NULL) {" <ident> "(pp); *pp = NULL; }" "}"

lowered_init
    = <passthrough>            (* when init_expr is not move_expr          *)
    | <ident> ";"              (* source variable, followed by NULLing it  *)
    ;
```

For `move_expr`, the lowering emits:

```c
type_ref "*" <dest> "..." "=" <source> ";"
<source> "= NULL;"                         (* ownership transferred        *)
```

Use-after-move: when the symbol table records a variable as *moved*, the
transpiler injects before the offending statement:

```c
"assert(" <ident> " != NULL && \"use-after-move: '" <ident> "'\");"
```

**Error codes:**

| Code | Trigger |
| ------ | --------- |
| E201 | `unique` used as an identifier |
| E202 | `unique` declaration without an initializer |
| E203 | Assignment of a `unique` variable to a non-`unique` pointer (implicit copy) |
| E204 | `move()` used outside the initializer of a `unique` declaration |

---

## v3 grammar (planned — not yet implemented)

v3 extends `struct_decl` to support **function prototypes** inside the struct
body, and introduces `Type.fn()` **definition syntax** outside the struct.

### Struct with method prototypes

```ebnf
struct_decl_v3
    = "struct" <ident> "{" { struct_member } "}" ";"
    ;

struct_member
    = fn_proto                 (* extracted; not emitted as a field        *)
    | <passthrough>            (* field declaration — verbatim C23         *)
    ;

fn_proto
    = [ "static" ] type_ref <ident> "(" param_list ")" ";"
    ;

param_list
    = <passthrough>            (* parameter list — verbatim C23            *)
    ;
```

**Lowering of `struct_decl_v3`:**

1. `typedef struct Name Name;` + `struct Name { <fields only> };` emitted as in v2.
2. Each `fn_proto` extracted from the body and emitted after the struct:
   - Without `static`: `ReturnType TypeName_fnName ( param_list ) ;`
   - With `static`: `static ReturnType typename_fnname ( param_list ) ;`

Visibility convention:

- Public prototype → `TypeName_fnName` (PascalCase prefix matches the struct name)
- Private prototype (`static`) → `static typename_fnname` (lowercase prefix)

---

### Method definition syntax

```ebnf
fn_definition
    = [ "static" ] type_ref <ident> "." <ident> "(" param_list ")" compound_stmt
    ;

compound_stmt
    = "{" <passthrough> "}"    (* body — verbatim C23                      *)
    ;
```

The `<ident> "." <ident>` pattern is the **definition syntax**: the first
`<ident>` is the struct type name, the second is the method name.

**Lowering of `fn_definition`:**

```ebnf
[ "static" ] type_ref TypeName "_" fnName "(" param_list ")" compound_stmt
```

Examples:

| cplus source | generated C |
| --- | --- |
| `Person *Person.new(...)  { ... }` | `Person *Person_new(...) { ... }` |
| `static void Person.helper() { ... }` | `static void person_helper() { ... }` |

> **Note on dot-call (`obj.method(args)`) :** rewriting call sites requires
> type inference (resolving the type of `obj` from the symbol table).  This is
> **out of scope for v3** and targeted at v5+.

---

## v5+ grammar (informative — subject to change)

v5+ introduces dot-call syntax, which requires type inference across expressions.

```ebnf
dot_call_expr
    = <ident> "." <ident> "(" arg_list ")"
    ;

arg_list
    = <passthrough>
    ;
```

**Lowering (tentative):**

```ebnf
dot_call_expr  →  TypeName_fnName "(" "&" <ident> [ "," arg_list ] ")"
```

This requires the transpiler to resolve the declared type of `<ident>` from
the symbol table — a full cross-expression type tracker, not available in the
island parser.

---

## Scope tracking summary

| Construct | Condition for recognition |
| ----------- | -------------------------- |
| `struct_decl` | `brace_depth == 0` |
| `weak_decl` | any scope |
| `unique_decl` | any scope |
| `move_expr` | only as `init_expr` of `unique_decl` |
| `fn_proto` (v3) | inside `struct_decl` body (`brace_depth == 1`) |
| `fn_definition` (v3) | `brace_depth == 0` |
| `dot_call_expr` (v5+) | any expression context; requires type inference |

---

## Error catalogue

| Code | Version | Message | Trigger |
| ------ | --------- | --------- | --------- |
| E201 | v2 | `reserved keyword used as identifier: 'X'` | `int weak = 0;` |
| E202 | v2 | `unique declaration requires an initializer` | `unique(fn) T *p;` |
| E203 | v2 | `cannot copy a unique pointer: 'X'` | `T *q = unique_p;` |
| E204 | v2 | `move() is only valid as a unique initializer` | `fn(move(p))` |
