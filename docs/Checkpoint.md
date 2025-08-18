# Checkpoint — August 2025  

## Current Status
- **gen_parser**  
  - `main.c` updated.  
  - Now accepts options:  
    - `-f <file>`: parse a file.  
    - `-x "<expr>"`: parse an inline expression.  
    - `-G`: print the grammar.  
  - Structure fixed:  
    - Parsers created with `mpc_new`.  
    - `mpca_lang` called with parsers in the correct order.  
    - Proper cleanup with `mpc_cleanup`, listing all parsers explicitly.  
  - AST printing:  
    - `print_program(ast)` for “pretty” AST output.  
    - `mpc_ast_print(ast)` for debug output.  

- **grammar.mpc**  
  - Simplified grammar, focused on **classes** and **interfaces**, based on the specs and test files.  
  - `access_mod` as an optional prefix for fields/methods.  
  - `method_decl` and `field_decl` follow standard C-like syntax.  
  - `class_decl` and `interface_decl` implemented.  
  - Support for forward declarations and `typedef`.  
  - `program` defined as a sequence of `decl`.  
  - Tested successfully with `tests/00_basic_interface.cplus.h` and inline examples.  

- **Tests**  
  - `00_basic_interface.cplus.h` validated.  
  - Parser compiles without errors.  
  - Previous issues (stray `#`, conflicts with `(`) fixed.  
  - Grammar printing confirmed via `-G`.  

## Next Steps
1. **Run additional prepared tests**.  
2. Expand the grammar with:  
   - Support for `static` (class variables).  
   - `typedef class` / `typedef interface` as defaults.  
   - Rules for `IBar *x;` (allowing interface pointers).  
   - Semantics of `as` (casting/interfaces).  
3. Confirm unified grammar for both headers and sources.  
4. Integrate with AST and transpiler.  
