# cplus v1 Architecture

## Scope

Version 1 is an identity transpiler for C23:

- Input: C23 source file
- Output: Equivalent C23 source file
- No language extensions in this version

## Design goals

- Keep the pipeline simple and testable
- Use existing compilers as syntax oracles
- Produce human-readable output
- Keep extension points for future cplus features

## Pipeline

1. Load source file
2. Run syntax validation with:
   - GCC: `gcc -std=c23 -fsyntax-only`
   - Clang: `clang -std=c23 -fsyntax-only`
3. Normalize diagnostics
4. If valid, emit identity output
5. Return non-zero exit code on errors

## Modules

- `driver`: CLI parsing and orchestration
- `io`: file read/write utilities
- `validator`: compiler invocation and status parsing
- `diagnostics`: normalized error model and printing
- `pipeline`: high-level workflow API

## Non-goals (v1)

- Full custom C parser
- Formatting preservation beyond identity copy
- Any cplus extension syntax

## Extension path (v2+)

- Add pre-lowering stage for cplus constructs
- Keep post-lowering validation with GCC/Clang
- Add source mapping for transformed diagnostics
