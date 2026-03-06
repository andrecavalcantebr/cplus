# Build Flags Policy

## Language mode

- Use strict C23:
  - `-std=c23`
- Do not use GNU extensions:
  - avoid `-std=gnu23`
  - `CMAKE_C_EXTENSIONS` must be `OFF`

## Safety and diagnostics flags

### GCC baseline

- `-Wall -Wextra -Wpedantic`
- `-Wformat=2`
- `-Warray-bounds=2`
- `-Wstringop-overflow=4`
- `-Wstringop-truncation`
- `-Wformat-overflow=2`
- `-Wformat-truncation=2`
- `-Walloc-size-larger-than=1048576`
- `-Wvla`
- `-fstack-protector-strong`
- `-D_FORTIFY_SOURCE=3`

### Clang baseline

- `-Wall -Wextra -Wpedantic`
- `-Wformat=2`
- `-Warray-bounds`
- `-Wvla`
- `-fstack-protector-strong`
- `-D_FORTIFY_SOURCE=3`

## Runtime sanitizers (Debug)

- `-fsanitize=address,undefined`
- `-fno-omit-frame-pointer`

## Linker libraries

Project baseline linker libraries:

- `-lm`
- `-lpthread`
