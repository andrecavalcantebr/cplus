# Especificação Resumida do Cplus

## Objetivo
Extensão mínima de C23 para permitir programação orientada a objetos.

Transpila para C puro legível e compilável por qualquer compilador C23.

Prioriza composição e agregação sobre herança.

Herança simples (apenas uma base) + interfaces múltiplas.

Encapsulamento gerido no transpiler — código C gerado expõe tudo, mas regras são checadas no Cplus.

## Palavras reservadas

### class
Define uma classe. Não mexe com a definição de struct padrão do C. Para você fazer uma classe em Cplus
você usa `class` e não `struct`

**Sintaxe básica:**

```Java 
class Foo {
    // members declaration
}
```

### interface
Define uma interface. Uma `interface` só tem métodos públicos, portanto não admitem outros modificadores

**Sintaxe básica:**

```Java 
interface IFoo {
    // methods declaration
}
```

### public, protected, private
Modificadores de acesso dos membros

**Sintaxe básica:**

```C++ 
class Foo {
    public int f1(Foo *self);
    protected char *getStr(Foo *self);
    private char *msg;
}
```

### extends
Usado para definir a classe base no caso de herança em uma classe

**Sintaxe básica:**
```Java
class Foo extends Bar {
    // members declaration
}
```

### implements
Usado para definir o conjunto de interfaces implementadas em uma classe

**Sintaxe básica:**

```Java
class Foo implements IClone, IBar {
    // members declaration
}
```

### as
Usado para _cast_ estático

**Sintaxe básica:**

```Java
auto c = obj as IClone
```

### new, del


# Regras

* Classe que implementa interface deve fornecer todas as implementações.

* Visibilidade

  * `public` → acessível por qualquer código.

  * `protected` → acessível pela própria classe e subclasses.

  * `private` → acessível apenas pela própria classe.

* Interfaces 

  * apenas `public`.

  * duas interfaces com o mesmo nome de método devem ter a mesma assinatura completa

  * uma classe que implementa duas interfaces com o mesmo nome deve implementar somente um código

  * apenas uma entrada na vtable

  * uma vtable por classe

* Herança

  * Simples: apenas uma extends.

  * Layout: base é o primeiro campo da classe derivada (compatível com upcast por ponteiro).

  * Override automático: métodos public/protected da base com mesma assinatura na derivada sobrescrevem na vtable.

* Membros

  * De instância: armazenados em cada objeto.

  * De classe (static): armazenados globalmente, não na instância da classe.

* Métodos:

  * Normais → chamadas diretas no C.

  * Virtuais → ponteiros na vtable.

  * Por simplicidade, padrão é virtual.

* Sobrecarga:

Em estudo


# Criação e destruição de objetos

**Heap:**

```C++
Motor *m = new(Motor, 5000);
del(m);
```

**Gera:**

```C++
Motor *cplus_new(ClassMeta *meta, ...)

void cplus_del(void *obj)
```


**Stack:**

```C++
Motor m;
init(&m, 5000);
deinit(&m);
```

# Inicialização

Dois níveis:

`sys_init()` / `sys_deinit()` → gerados pelo transpiler (seta vtables, overrides, meta).

`init() / deinit()` → definidos pelo usuário, chamam sys_* automaticamente no prólogo/epílogo.

# RTTI & Meta
Cada classe gera um objeto ClassMeta:

```C++
typedef struct {
    const char *name;
    size_t size;
    const ClassMeta *base;
    // vtables por interface e classe
} ClassMeta;
```

Usado por `cplus_new()` e para casts seguros (`as`).

Transpiler injeta meta nas instâncias no campo Class.

# Casts

Classe → interface:

```C++
IStartable *ist = m as IStartable;
```

Classe derivada → base:

```C++
Device *d = m as Device;
```

Checagem no transpiler; C gerado faz cast simples.

# Arquivos e separação

`.h` → `class` + declarações de métodos.

`.cp` → implementações.

Permite composição de vários arquivos no padrão C.

# Geração de código C

Todas as visibilidades viram struct pública no C.

Encapsulamento é garantido pelo transpiler, não em runtime.

Métodos virtuais → ponteiros na vtable.

`#line` usado para mapear _erros/warnings_ ao `.cplus`.

Ida: linha real no `.cplus`.

Volta: #line 1 "Classe.gen.c".

# Convenções

**Nome gerado no C:**

`Class_metodo` para normais.

`Class__sys_init`, `Class__sys_deinit` gerados.

Métodos virtuais na vtable com assinatura fixa para a classe.
