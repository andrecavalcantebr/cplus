# cplus Lint

VS Code extension that provides real-time syntax checking for `.cplus` and `.hplus` source files using GCC or Clang as the validation backend.

## Features

- Errors and warnings appear inline as you type (or on save).
- Supports both `.cplus` (implementation) and `.hplus` (header) files.
- Syntax highlighting via the bundled C grammar.
- GCC < 14 compatibility shim: `-std=c23` is automatically remapped to `-std=c2x` at runtime.
- Configurable compiler, C standard, debounce delay, and lint trigger.

## Requirements

GCC or Clang must be installed and available on `PATH`.

```
gcc --version   # e.g. gcc (Ubuntu) 14.2.0
clang --version # optional
```

## Extension Settings

| Setting | Type | Default | Description |
|---------|------|---------|-------------|
| `cplusLint.compiler` | `"gcc"` \| `"clang"` | `"gcc"` | Compiler used for validation. |
| `cplusLint.std` | string | `"c23"` | C standard flag (e.g. `c23`, `c2x`, `c17`). |
| `cplusLint.lintOnChange` | boolean | `true` | Lint while typing. When `false`, lints only on save. |
| `cplusLint.lintDelay` | number | `600` | Debounce delay in milliseconds before linting after a change. |

Example (`settings.json`):

```json
{
  "cplusLint.compiler": "gcc",
  "cplusLint.std": "c23",
  "cplusLint.lintOnChange": true,
  "cplusLint.lintDelay": 400
}
```

## How it works

1. On open, change, or save of a `.cplus`/`.hplus` file, the extension writes the current buffer to a temporary file.
2. It invokes `gcc -x c -std=<std> -I <document-dir> -I <workspace-folder> -fsyntax-only <tmpfile>` (or the configured compiler).
3. Raw compiler output is parsed for lines matching `<file>:<line>:<col>: (error|warning|note): <message>`.
4. Diagnostics are mapped back to the original document and shown in the Problems panel and inline.

No full compilation is performed — only syntax and type checking (`-fsyntax-only`).

## Installation

Install from the `.vsix` package:

```bash
cd vscode-cplus-lint
vsce package          # produces vscode-cplus-lint-0.1.0.vsix
code --install-extension vscode-cplus-lint-0.1.0.vsix
```

Or open the folder in VS Code and press **F5** to launch the Extension Development Host directly.

## Known Limitations

- The lint command adds the current document directory and each workspace root as include paths. Projects that depend on additional generated/include directories may still need broader compiler integration.
- cplus v2 keywords (`weak`, `unique`, `move`, `class`) are not yet in the vocabulary of the GCC/Clang frontend, so they will appear as errors until the cplus transpiler generates the C output. This is expected — the lint extension validates the C23 subset only.

## License

GPL-v3 — see the root [LICENSE](../LICENSE) file.
