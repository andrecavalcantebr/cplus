📜 Especificação Resumida do Cplus
1. Objetivo
Extensão mínima de C23 para permitir programação orientada a objetos.

Transpila para C puro legível e compilável por qualquer compilador C23.

Prioriza composição e agregação sobre herança.

Herança simples (apenas uma base) + interfaces múltiplas.

Encapsulamento gerido no transpiler — código C gerado expõe tudo, mas regras são checadas no Cplus.

2. Palavras reservadas
typescript
Copiar
Editar
class
interface
public
protected
private
extends
implements
as         // cast estático para interface ou classe base
new
del
3. Estrutura de classes
3.1 Declaração
cplus
Copiar
Editar
class Motor extends Device implements IStartable, IStopable {
public:
    void init(Motor *self, int rpm);
protected:
    int rpm;
private:
    bool started;
};
3.2 Interfaces
cplus
Copiar
Editar
interface IStartable {
    void start(IStartable *self);
};
Interfaces só têm métodos públicos abstratos.

Não possuem atributos.

Classe que implementa interface deve fornecer todas as implementações.

4. Visibilidade
public → acessível por qualquer código.

protected → acessível pela própria classe e subclasses.

private → acessível apenas pela própria classe.

Interfaces → apenas public.

5. Herança
Simples: apenas uma extends.

Layout: base é o primeiro campo da struct derivada (compatível com upcast por ponteiro).

Override automático: métodos public/protected da base com mesma assinatura na derivada sobrescrevem na vtable.

6. Membros
De instância: armazenados em cada objeto.

De classe (static): armazenados globalmente, não no struct.

Métodos:

Normais → chamadas diretas no C.

Virtuais → ponteiros na vtable.

Por simplicidade, padrão é virtual.

Sobrecarga:

Permitida apenas por número de parâmetros.

Interfaces não podem ter sobrecarga.

Gerado no C como _Generic + métodos renomeados (__1, __2, etc.).

7. Criação e destruição
Heap:
cplus
Copiar
Editar
Motor *m = new(Motor, 5000);
del(m);
Gera:

Motor *cplus_new(ClassMeta *meta, ...)

void cplus_del(void *obj)

Stack:
cplus
Copiar
Editar
Motor m;
init(&m, 5000);
deinit(&m);
8. Inicialização
Dois níveis:

sys_init() / sys_deinit() → gerados pelo transpiler (seta vtables, overrides, meta).

init() / deinit() → definidos pelo usuário, chamam sys_* automaticamente no prólogo/epílogo.

9. RTTI & Meta
Cada classe gera um objeto ClassMeta:

c
Copiar
Editar
typedef struct {
    const char *name;
    size_t size;
    const ClassMeta *base;
    // vtables por interface e classe
} ClassMeta;
Usado por cplus_new() e para casts seguros (as).

Transpiler injeta meta nas instâncias no campo Class.

10. Casts
Classe → interface:

cplus
Copiar
Editar
IStartable *ist = m as IStartable;
Classe derivada → base:

cplus
Copiar
Editar
Device *d = m as Device;
Checagem no transpiler; C gerado faz cast simples.

11. Arquivos e separação
.h → struct + declarações de métodos.

.cp → implementações.

Permite composição de vários arquivos no padrão C.

12. Geração de código C
Todas as visibilidades viram struct pública no C.

Encapsulamento é garantido pelo transpiler, não em runtime.

Métodos virtuais → ponteiros na vtable.

#line usado para mapear erros/warnings ao .cplus.

Ida: linha real no .cplus.

Volta: #line 1 "Classe.gen.c".

13. Convenções
Nome gerado no C:

Classe_metodo para normais.

Classe__sys_init, Classe__sys_deinit gerados.

Métodos sobrecarregados → Classe_metodo__1, __2 etc.

Métodos virtuais na vtable com assinatura fixa para a classe.