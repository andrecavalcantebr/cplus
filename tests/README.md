# Cplus Tests — Grupo 07..14

AST esperada (alto nível) para cada arquivo, conforme gramática:

- `program : <decl>*`
- `decl : typedef_class | typedef_interface | forward_class | forward_interface | interface | class | c_chunk`
- Nós internos relevantes: `interface`, `class`, `class_tail`, `interface_item` (→ `method_sig`), `class_item` (→ `member_decl`), `member_decl` (→ `access_mods_opt`, `storage_mods_opt`, `type_name`, `identifier`, `member_after_name`), `c_chunk`.

> Observação: As árvores abaixo mostram apenas a **forma** que o parser deve produzir, não os offsets/locs.

---

## 07_interface_basic.cplus.h

```
program
  interface
    identifier: IStartable
    interface_item
      method_sig
        type_name: int
        identifier: start
        param_list: (void)
    interface_item
      method_sig
        type_name: void
        identifier: stop
        param_list: ()
    c_comment  // "// comment preserved"
```

---

## 08_class_access_static.cplus.h

```
program
  class
    identifier: Device
    class_tail: ()
    class_item
      member_decl
        access_mods_opt: public
        storage_mods_opt: static
        type_name: int
        identifier: version
        member_after_name: method()
    class_item
      member_decl
        access_mods_opt: public
        type_name: int
        identifier: id
        member_after_name: field
    class_item
      member_decl
        access_mods_opt: protected
        storage_mods_opt: static
        type_name: int
        identifier: live_count
        member_after_name: field
    class_item
      member_decl
        access_mods_opt: private
        type_name: char *
        identifier: name
        member_after_name: field
    class_item
      member_decl
        access_mods_opt: public
        type_name: int
        identifier: rename
        member_after_name: method(Device* self, const char* s)
```

---

## 09_extends_implements.cplus.h

```
program
  interface IFoo
    interface_item: method_sig (int foo(void))
  interface IBar
    interface_item: method_sig (void bar(int x))

  class Base
    class_item: member_decl (public int baseMethod(Base* self))

  class Derived
    class_tail: extends Base implements IFoo, IBar
    class_item: member_decl (private int state;)
    class_item: member_decl (public int foo(void);)
    class_item: member_decl (public void bar(int x);)
```

---

## 10_generics_typename_arrays.cplus.h

```
program
  interface IVec<T>
    interface_item: method_sig (T getAt(IVec<T>* self, int i))
    interface_item: method_sig (int size(void))

  class Vector<T>
    class_item: member_decl (public T* data;)
    class_item: member_decl (public int length;)
    class_item: member_decl (public static Vector<T>* create(int n);)
    class_item: member_decl (public T getAt(Vector<T>* self, int i);)

  class Matrix<T>
    class_item: member_decl (public T** cells[];)
    class_item: member_decl (public int dims[3];)
    class_item: member_decl (public T get(Matrix<T>* self, int i, int j);)
```

---

## 11_typedef_class_interface.cplus.h

```
program
  typedef_interface ICounter {
    int inc(ICounter* self);
    int value(ICounter* self);
  } ICounter;

  typedef_class Counter {
    private int v;
    public static Counter* create(int init);
    public int inc(Counter* self);
    public int value(Counter* self);
  } Counter;
```

---

## 12_forwards_and_use.cplus.h

```
program
  forward_interface IService;
  forward_class Impl;

  interface IService {
    int run(IService* self);
  }

  class Impl implements IService {
    private int code;
    public int run(IService* self);
  }
```

---

## 13_c_chunk_mixed.cplus.h

```
program
  c_comment        // // C preprocessor line:
  c_pp_line        // #define SOME_MACRO 42
  c_comment        // // C declaration-like line:
  c_decl_like      // extern int some_c_symbol;
  c_comment        // // Function pointer typedef ...
  c_decl_like      // typedef int (*cmp_fn)(...);
  c_comment        // // Struct tag line (C-ish):
  c_decl_like      // struct FooTag;
```

---

## 14_arrays_pointers_mix.cplus.h

```
program
  interface IStore<T>
    interface_item: method_sig (int put(IStore<T>* self, T item))
    interface_item: method_sig (T get(IStore<T>* self, int idx))

  class Store<T>
    class_item: member_decl (private T* buffer[4];)
    class_item: member_decl (private int used;)
    class_item: member_decl (public static Store<T>* create(int cap);)
    class_item: member_decl (public int put(Store<T>* self, T item);)
    class_item: member_decl (public T get(Store<T>* self, int idx);)
```
