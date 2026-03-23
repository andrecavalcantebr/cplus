# cplus v1 Specification

## File extensions

| Extension | Role | Written by |
|-----------|------|------------|
| `.hplus`  | cplus interface source (declarations, types) | programmer |
| `.cplus`  | cplus implementation source | programmer |
| `.h`      | generated C header — do not edit | transpiler |
| `.c`      | generated C source — do not edit | transpiler |

C standard headers (`.h`) from external libraries are consumed as-is and are
never transpiled. Only `.hplus` / `.cplus` files are transpiler inputs.

## CLI

```text
cplus <file.hplus|file.cplus> [...] [-o <output>] [--cc gcc|clang] [--std c23]
```

Options:

| Flag | Description | Default |
|------|-------------|---------|
| `-o <output>` | output path; only valid with a single input file | see table below |
| `--cc` | compiler used for syntax validation | `gcc` |
| `--std` | C standard passed to the compiler | `c23` |

Default output names (when `-o` is omitted):

| Input | Output |
|-------|--------|
| `foo.hplus` | `foo.h` |
| `foo.cplus` | `foo.c` |

Examples:

```bash
# transpile each file individually (most common usage)
cplus person.hplus person.cplus main.cplus

# explicit output (single file only)
cplus person.hplus -o person.h

# choose compiler and standard
cplus person.cplus --cc clang --std c23
```

## Behavior

- Validate input syntax using selected compiler
- On success:
  - write output file
  - default output: `<input>.out.c` if `-o` is not provided
- On failure:
  - print normalized diagnostics
  - do not emit output file
  - return exit code `1`

## Exit codes

- `0`: success
- `1`: validation or input error
- `2`: internal runtime error

## Diagnostic format

```text
<file>:<line>:<column>: <severity>: <message>
```

Severity values:

- `error`
- `warning`
- `note`

## v1 transformation

No semantic transformation is applied.
Current lowering is identity copy.
