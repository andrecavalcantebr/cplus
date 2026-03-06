# cplus v1 Specification

## CLI

```text
cplus <input.c> [-o <output.c>] [--cc gcc|clang] [--std c23]
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
