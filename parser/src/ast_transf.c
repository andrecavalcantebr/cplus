/*
    FILE: ast_transf.c
    DESCRIPTION: Minimal, incremental AST -> C code generator for Cplus
    AUTHOR: Andre Cavalcante
    DATE: August, 2025
    LICENSE: CC BY-SA
*/

#define _GNU_SOURCE
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* =========================================================================
 * Small dynamic vectors (incremental: simple, no dependencies)
 * ========================================================================= */
typedef struct
{
    char *type;
    char *name;
} IR_Param;

typedef struct
{
    char *ret_type;
    char *name;
    IR_Param *params;
    size_t nparams;
    int is_static;
} IR_Method;

typedef struct
{
    char *type;
    char *name;
    int is_static;
} IR_Field;

typedef struct
{
    char *name;
    IR_Method *methods;
    size_t nmethods;
} IR_Interface;

typedef struct
{
    char *name;
    /* Future: char *base; char **ifaces; size_t nifaces; */
    IR_Field *fields;
    size_t nfields;
    IR_Method *methods;
    size_t nmethods;
} IR_Class;

typedef struct
{
    IR_Interface *ifaces;
    size_t nifaces;
    IR_Class *classes;
    size_t nclasses;
} IR_Module;

static void *xrealloc(void *p, size_t n, size_t sz)
{
    void *q = realloc(p, n * sz);
    if (!q && n)
    {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return q;
}
static char *xstrdup(const char *s)
{
    if (!s)
        return NULL;
    size_t n = strlen(s);
    char *d = (char *)malloc(n + 1);
    if (!d)
    {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    memcpy(d, s, n + 1);
    return d;
}

/* =========================================================================
 * MPC AST helpers (robust against small grammar changes)
 * ========================================================================= */
/* static int has_tag(const mpc_ast_t *n, const char *tag)
{
    return n && n->tag && strstr(n->tag, tag) != NULL;
} */

/* Recursively collect visible text (used by the fallback line parser) */
static void collect_text_rec(const mpc_ast_t *n, char *buf, size_t cap)
{
    if (!n || !buf || cap == 0)
        return;
    if (n->children_num == 0)
    {
        if (n->contents && *n->contents)
        {
            size_t bl = strlen(buf), nl = strlen(n->contents);
            if (bl < cap - 1)
            {
                size_t cp = nl;
                if (bl + cp >= cap)
                    cp = cap - bl - 1;
                memcpy(buf + bl, n->contents, cp);
                buf[bl + cp] = '\0';
            }
        }
        return;
    }
    for (int i = 0; i < n->children_num; i++)
        collect_text_rec(n->children[i], buf, cap);
}

/* Compact whitespace of a node into out buffer */
/* static void node_text(const mpc_ast_t *n, char *out, size_t cap)
{
    if (!out || cap == 0)
        return;
    out[0] = '\0';
    collect_text_rec(n, out, cap);
    size_t w = 0;
    for (size_t r = 0; out[r] && w + 1 < cap; r++)
    {
        unsigned char ch = (unsigned char)out[r];
        if (isspace(ch))
        {
            if (w && out[w - 1] != ' ')
                out[w++] = ' ';
        }
        else
        {
            out[w++] = out[r];
        }
    }
    out[w] = '\0';
} */

/* Collects relevant lines from c_chunk (decl lines/multilines and regex payload) */
static void collect_text_from_c_chunk(const mpc_ast_t *n, FILE *out)
{
    if (!n)
        return;
    if (n->tag && strstr(n->tag, "c_decl_"))
    {
        char buf[2048] = {0};
        collect_text_rec(n, buf, sizeof buf);
        if (buf[0])
        {
            fputs(buf, out);
            fputc('\n', out);
        }
        return;
    }
    if (n->tag && strstr(n->tag, "regex"))
    {
        if (n->contents && *n->contents)
        {
            fputs(n->contents, out);
            fputc('\n', out);
        }
        return;
    }
    for (int i = 0; i < n->children_num; i++)
        collect_text_from_c_chunk(n->children[i], out);
}

/* =========================================================================
 * Tiny “line” parser (fallback) – tolerant, works while grammar tags are c_chunk
 * ========================================================================= */
static const char *skip_ws(const char *s)
{
    while (*s && isspace((unsigned char)*s))
        s++;
    return s;
}
static const char *read_ident(const char *s, char *out, size_t cap)
{
    s = skip_ws(s);
    size_t i = 0;
    if (*s == '\0' || !(isalpha((unsigned char)*s) || *s == '_'))
        return NULL;
    out[i++] = *s++;
    while (*s && (isalnum((unsigned char)*s) || *s == '_'))
    {
        if (i + 1 < cap)
            out[i++] = *s;
        s++;
    }
    out[i] = '\0';
    return s;
}

static int parse_interface_header(const char *ln, char *name, size_t ncap)
{
    const char *s = skip_ws(ln);
    if (strncmp(s, "interface", 9) != 0 || !isspace((unsigned char)s[9]))
        return 0;
    s += 9;
    const char *p = read_ident(s, name, ncap);
    if (!p)
        return 0;
    p = strchr(p, '{');
    return p != NULL;
}

static int parse_class_header(const char *ln, char *name, size_t ncap,
                              char *base, size_t bcap, char *impls, size_t icap)
{
    base[0] = impls[0] = '\0';
    const char *s = skip_ws(ln);
    if (strncmp(s, "class", 5) != 0 || !isspace((unsigned char)s[5]))
        return 0;
    s += 5;
    const char *p = read_ident(s, name, ncap);
    if (!p)
        return 0;

    /* Optional “extends” / “implements” – stored but not used yet */
    char word[64];
    while ((p = skip_ws(p)) && *p && *p != '{')
    {
        const char *q = read_ident(p, word, sizeof word);
        if (!q)
            break;
        if (strcmp(word, "extends") == 0)
        {
            q = read_ident(q, base, bcap);
            if (!q)
                break;
            p = q;
            continue;
        }
        if (strcmp(word, "implements") == 0)
        {
            q = strchr(q, '{');
            size_t len = (q ? (size_t)(q - p) : strlen(p));
            if (len >= icap)
                len = icap - 1;
            memcpy(impls, p, len);
            impls[len] = '\0';
            break;
        }
        p = q;
    }
    return strchr(p ? p : ln, '{') != NULL;
}

/* Member line: (1) method if contains '('  (2) field otherwise */
static int parse_member_line(const char *ln, IR_Field *outF, IR_Method *outM)
{
    const char *s = skip_ws(ln);
    if (*s == '}' || *s == '\0')
        return 0;

    /* strip trailing // comment for simplicity */
    const char *slash = strstr(s, "//");
    size_t n = slash ? (size_t)(slash - s) : strlen(s);
    char tmp[512];
    if (n >= sizeof tmp)
        n = sizeof tmp - 1;
    memcpy(tmp, s, n);
    tmp[n] = '\0';

    if (!strchr(tmp, '('))
    {
        /* Field */
        char mod[16] = {0}, type[128] = {0}, name[128] = {0};
        const char *p = read_ident(tmp, mod, sizeof mod);
        if (!p)
            return 0;
        if (strcmp(mod, "public") == 0 || strcmp(mod, "protected") == 0 ||
            strcmp(mod, "private") == 0 || strcmp(mod, "static") == 0)
        {
            p = read_ident(p, type, sizeof type);
        }
        else
        {
            strcpy(type, mod);
        }
        p = read_ident(p, name, sizeof name);
        if (!p)
            return 0;
        outF->type = xstrdup(type);
        outF->name = xstrdup(name);
        outF->is_static = 0;
        return 2; /* field */
    }
    else
    {
        /* Method */
        char mod[16] = {0}, rtype[128] = {0}, mname[128] = {0};
        const char *p = read_ident(tmp, mod, sizeof mod);
        if (!p)
            return 0;
        int had_mod = (strcmp(mod, "public") == 0 || strcmp(mod, "protected") == 0 ||
                       strcmp(mod, "private") == 0 || strcmp(mod, "static") == 0);
        if (had_mod)
        {
            p = read_ident(p, rtype, sizeof rtype);
        }
        else
        {
            strcpy(rtype, mod);
        }
        p = read_ident(p, mname, sizeof mname);
        if (!p)
            return 0;
        const char *lpar = strchr(p, '('), *rpar = lpar ? strchr(lpar, ')') : NULL;
        if (!lpar || !rpar)
            return 0;

        IR_Method M = (IR_Method){0};
        M.ret_type = xstrdup(rtype);
        M.name = xstrdup(mname);
        M.is_static = (strcmp(mod, "static") == 0);

        /* Split args by ',' inside (...) */
        char args[256] = {0};
        size_t len = (size_t)(rpar - lpar - 1);
        if (len >= sizeof args)
            len = sizeof args - 1;
        memcpy(args, lpar + 1, len);
        args[len] = '\0';

        char *save = NULL;
        for (char *tok = strtok_r(args, ",", &save); tok; tok = strtok_r(NULL, ",", &save))
        {
            char tbuf[128] = {0}, nbuf[128] = {0};
            const char *pp = skip_ws(tok);
            if (strncmp(pp, "class ", 6) == 0)
                pp += 6; /* allow "class Motor *self" */

            /* Find last identifier (name); rest is type */
            const char *last = pp + strlen(pp);
            while (last > pp && isspace((unsigned char)last[-1]))
                last--;
            size_t i = 0;
            while (last > pp && (isalnum((unsigned char)last[-1]) || last[-1] == '_'))
            {
                last--;
                i++;
            }
            if (i == 0)
            {
                strcpy(tbuf, "void");
                strcpy(nbuf, "arg");
            }
            else
            {
                size_t nlen = i;
                if (nlen >= sizeof nbuf)
                    nlen = sizeof nbuf - 1;
                memcpy(nbuf, last, nlen);
                nbuf[nlen] = '\0';
                size_t tlen = (size_t)(last - pp);
                if (tlen >= sizeof tbuf)
                    tlen = sizeof tbuf - 1;
                memcpy(tbuf, pp, tlen);
                tbuf[tlen] = '\0';
                /* compact type spaces */
                char clean[128] = {0};
                size_t w = 0;
                for (size_t r = 0; tbuf[r] && w + 1 < sizeof clean; r++)
                {
                    unsigned char ch = (unsigned char)tbuf[r];
                    if (isspace(ch))
                    {
                        if (w && clean[w - 1] != ' ')
                            clean[w++] = ' ';
                    }
                    else
                        clean[w++] = tbuf[r];
                }
                strcpy(tbuf, clean[0] ? clean : "void");
            }
            IR_Param P = {.type = xstrdup(tbuf[0] ? tbuf : "void"),
                          .name = xstrdup(nbuf[0] ? nbuf : "arg")};
            M.params = (IR_Param *)xrealloc(M.params, M.nparams + 1, sizeof(IR_Param));
            M.params[M.nparams++] = P;
        }
        *outM = M;
        return 1; /* method */
    }
}

/* Parse interfaces/classes from flattened text (fallback) */

static void drop_leading_self(IR_Method *m, const char *owner_class)
{
    if (!m || m->nparams == 0)
        return;
    IR_Param *p0 = &m->params[0];

    /* Tem que se chamar self; se não, não é o caso. */
    if (!p0->name || strcmp(p0->name, "self") != 0)
        return;

    /* Se for método de classe (não estático), a gente SEMPRE injeta Class *self
       na geração. Então, se o primeiro parâmetro é "self", removemos SEM OLHAR
       o tipo (pode vir "void *self" por conta de interface). */
    (void)owner_class; /* não precisamos mais checar o tipo */
    free(p0->type);
    free(p0->name);
    memmove(m->params, m->params + 1, (m->nparams - 1) * sizeof(IR_Param));
    m->nparams--;
}

static void iface_push_method(IR_Interface *I, IR_Method m)
{
    I->methods = (IR_Method *)xrealloc(I->methods, I->nmethods + 1, sizeof(IR_Method));
    I->methods[I->nmethods++] = m;
}
static void class_push_method(IR_Class *C, IR_Method m)
{
    C->methods = (IR_Method *)xrealloc(C->methods, C->nmethods + 1, sizeof(IR_Method));
    C->methods[C->nmethods++] = m;
}
static void class_push_field(IR_Class *C, IR_Field f)
{
    C->fields = (IR_Field *)xrealloc(C->fields, C->nfields + 1, sizeof(IR_Field));
    C->fields[C->nfields++] = f;
}
static void ir_push_interface(IR_Module *M, IR_Interface I)
{
    M->ifaces = (IR_Interface *)xrealloc(M->ifaces, M->nifaces + 1, sizeof(IR_Interface));
    M->ifaces[M->nifaces++] = I;
}
static void ir_push_class(IR_Module *M, IR_Class C)
{
    M->classes = (IR_Class *)xrealloc(M->classes, M->nclasses + 1, sizeof(IR_Class));
    M->classes[M->nclasses++] = C;
}

static IR_Module parse_from_flat_text(const char *text)
{
    IR_Module M = (IR_Module){0};
    const char *p = text;

    while (*p)
    {
        const char *e = strchr(p, '\n');
        if (!e)
            e = p + strlen(p);
        size_t n = (size_t)(e - p);
        char ln[1024];
        if (n >= sizeof ln)
            n = sizeof ln - 1;
        memcpy(ln, p, n);
        ln[n] = '\0';

        char name[128], base[128], impls[256];
        if (parse_interface_header(ln, name, sizeof name))
        {
            IR_Interface I = {.name = xstrdup(name), .methods = NULL, .nmethods = 0};
            p = (*e ? e + 1 : e);
            for (;;)
            {
                const char *e2 = strchr(p, '\n');
                if (!e2)
                    e2 = p + strlen(p);
                size_t mlen = (size_t)(e2 - p);
                char ml[1024];
                if (mlen >= sizeof ml)
                    mlen = sizeof ml - 1;
                memcpy(ml, p, mlen);
                ml[mlen] = '\0';
                if (strstr(ml, "};"))
                {
                    p = (*e2 ? e2 + 1 : e2);
                    break;
                }
                IR_Field F;
                IR_Method Me;
                int kind = parse_member_line(ml, &F, &Me);
                if (kind == 1)
                {
                    drop_leading_self(&Me, NULL);
                    iface_push_method(&I, Me);
                }
                p = (*e2 ? e2 + 1 : e2);
            }
            ir_push_interface(&M, I);
            continue;
        }
        else if (parse_class_header(ln, name, sizeof name, base, sizeof base, impls, sizeof impls))
        {
            IR_Class C = {.name = xstrdup(name), .fields = NULL, .nfields = 0, .methods = NULL, .nmethods = 0};
            p = (*e ? e + 1 : e);
            for (;;)
            {
                const char *e2 = strchr(p, '\n');
                if (!e2)
                    e2 = p + strlen(p);
                size_t mlen = (size_t)(e2 - p);
                char ml[1024];
                if (mlen >= sizeof ml)
                    mlen = sizeof ml - 1;
                memcpy(ml, p, mlen);
                ml[mlen] = '\0';
                if (strstr(ml, "};"))
                {
                    p = (*e2 ? e2 + 1 : e2);
                    break;
                }
                IR_Field F;
                IR_Method Me;
                int kind = parse_member_line(ml, &F, &Me);
                if (kind == 1)
                {
                    drop_leading_self(&Me, C.name);
                    class_push_method(&C, Me);
                }
                else if (kind == 2)
                    class_push_field(&C, F);
                p = (*e2 ? e2 + 1 : e2);
            }
            ir_push_class(&M, C);
            continue;
        }

        p = (*e ? e + 1 : e);
    }
    return M;
}

/* =========================================================================
 * Codegen (headers/sources in current working directory)
 * ========================================================================= */
static char *to_macro(const char *name)
{
    size_t n = strlen(name);
    char *m = (char *)malloc(n + 2);
    for (size_t i = 0; i < n; i++)
    {
        char c = name[i];
        m[i] = (char)(isalnum((unsigned char)c) ? toupper((unsigned char)c) : '_');
    }
    m[n] = '\0';
    return m;
}

static void write_interface_header(const IR_Interface *I)
{
    char path[256];
    snprintf(path, sizeof path, "%s.h", I->name);
    FILE *f = fopen(path, "w");
    if (!f)
    {
        perror(path);
        return;
    }

    char *guard = to_macro(I->name);
    fprintf(f,
            "/*\n\tFILE: %s\n\tDESCRIPTION: public definitions of %s\n\tAUTHOR: Andre Cavalcante\n\tDATE: August, 2025\n\tLICENSE: CC BY-SA\n*/\n\n",
            path, I->name);
    fprintf(f, "#ifndef %s_H\n#define %s_H\n\n", guard, guard);
    fprintf(f, "\n#include <stdbool.h>\n\n");

    fprintf(f, "typedef struct %s_vtable {\n", I->name);
    for (size_t i = 0; i < I->nmethods; i++)
    {
        const IR_Method *m = &I->methods[i];
        fprintf(f, "    %s (*%s)(void *self", m->ret_type, m->name);
        for (size_t p = 0; p < m->nparams; p++)
            fprintf(f, ", %s %s", m->params[p].type, m->params[p].name);
        fprintf(f, ");\n");
    }
    fprintf(f, "} %s_vtable;\n\n", I->name);

    fprintf(f, "#endif /* %s_H */\n", guard);
    free(guard);
    fclose(f);
}

static void write_class_header(const IR_Class *C)
{
    char path[256];
    snprintf(path, sizeof path, "%s.gen.h", C->name);
    FILE *f = fopen(path, "w");
    if (!f)
    {
        perror(path);
        return;
    }

    char *guard = to_macro(C->name);
    fprintf(f,
            "/*\n\tFILE: %s\n\tDESCRIPTION: public definitions of %s\n\tAUTHOR: Andre Cavalcante\n\tDATE: August, 2025\n\tLICENSE: CC BY-SA\n*/\n\n",
            path, C->name);
    fprintf(f, "#ifndef %s_GEN_H\n#define %s_GEN_H\n\n", guard, guard);

    fprintf(f, "\n#include <stdbool.h>\n");
    fprintf(f, "\n#include <stdlib.h>\n\n");

    fprintf(f, "typedef struct %s {\n", C->name);
    fprintf(f, "    const struct %s__meta_s *meta;\n", C->name);
    for (size_t i = 0; i < C->nfields; i++)
        fprintf(f, "    %s %s;\n", C->fields[i].type, C->fields[i].name);
    fprintf(f, "} %s;\n\n", C->name);

    for (size_t i = 0; i < C->nmethods; i++)
    {
        const IR_Method *m = &C->methods[i];
        fprintf(f, "%s %s_%s(", m->ret_type, C->name, m->name);
        if (m->is_static)
        {
            if (m->nparams == 0)
                fputs("void", f);
            else
            {
                for (size_t p = 0; p < m->nparams; p++)
                    fprintf(f, "%s%s %s", (p ? ", " : ""), m->params[p].type, m->params[p].name);
            }
        }
        else
        {
            fprintf(f, "%s *self", C->name);
            for (size_t p = 0; p < m->nparams; p++)
                fprintf(f, ", %s %s", m->params[p].type, m->params[p].name);
        }
        fprintf(f, ");\n");
    }

    fprintf(f, "void %s__sys_init(void);\n", C->name);
    fprintf(f, "void %s__sys_deinit(void);\n", C->name);

    fprintf(f, "\n#endif /* %s_GEN_H */\n", guard);
    free(guard);
    fclose(f);
}

static void write_class_source(const IR_Class *C)
{
    char path[256];
    snprintf(path, sizeof path, "%s.gen.c", C->name);
    FILE *f = fopen(path, "w");
    if (!f)
    {
        perror(path);
        return;
    }

    fprintf(f,
            "/*\n\tFILE: %s\n\tDESCRIPTION: public definitions of %s\n\tAUTHOR: Andre Cavalcante\n\tDATE: August, 2025\n\tLICENSE: CC BY-SA\n*/\n\n",
            path, C->name);

    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "#include <stdlib.h>\n\n");

    fprintf(f, "#include \"%s.gen.h\"\n\n", C->name);

    fprintf(f, "struct %s__meta_s {\n", C->name);
    fprintf(f, "    const char *name;\n");
    fprintf(f, "    size_t size;\n");
    fprintf(f, "};\n\n");

    fprintf(f, "static const struct %s__meta_s %s__meta = { \"%s\", sizeof(%s) };\n\n",
            C->name, C->name, C->name, C->name);

    for (size_t i = 0; i < C->nmethods; i++)
    {
        const IR_Method *m = &C->methods[i];
        fprintf(f, "%s %s_%s(", m->ret_type, C->name, m->name);
        if (m->is_static)
        {
            if (m->nparams == 0)
                fputs("void", f);
            else
            {
                for (size_t p = 0; p < m->nparams; p++)
                    fprintf(f, "%s%s %s", (p ? ", " : ""), m->params[p].type, m->params[p].name);
            }
        }
        else
        {
            fprintf(f, "%s *self", C->name);
            for (size_t p = 0; p < m->nparams; p++)
                fprintf(f, ", %s %s", m->params[p].type, m->params[p].name);
        }
        fprintf(f, ")\n{\n");
        fprintf(f, "    /* TODO: generated body */\n");
        if (strcmp(m->ret_type, "void") == 0)
        {
            fprintf(f, "    return;\n");
        }
        else
        {
            fprintf(f, "    %s r = 0; /* default */\n    return r;\n", m->ret_type);
        }
        fprintf(f, "}\n\n");
    }

    fprintf(f, "void %s__sys_init(void) {\n", C->name);
    fprintf(f, "    (void)%s__meta; /* TODO: set up vtables and meta */\n", C->name);
    fprintf(f, "}\n\n");

    fprintf(f, "void %s__sys_deinit(void) {\n", C->name);
    fprintf(f, "    /* TODO: tear down */\n");
    fprintf(f, "}\n");

    fclose(f);
}

/* =========================================================================
 * Public entrypoint
 * ========================================================================= */
void ast_transformation(mpc_ast_t *ast)
{
    /* Fallback textual path: useful while grammar emits c_chunk */
    char *flat = NULL;
    size_t flen = 0;
    FILE *mem = open_memstream(&flat, &flen);
    if (!mem)
    {
        perror("open_memstream");
        return;
    }
    collect_text_from_c_chunk(ast, mem);
    fclose(mem);

    IR_Module M = parse_from_flat_text(flat ? flat : "");
    free(flat);

    /* Emit interfaces */
    for (size_t i = 0; i < M.nifaces; i++)
        write_interface_header(&M.ifaces[i]);

    /* Emit classes (.h/.c) */
    for (size_t i = 0; i < M.nclasses; i++)
    {
        write_class_header(&M.classes[i]);
        write_class_source(&M.classes[i]);
    }

    /* NOTE: frees omitted for MVP; okay for a one-shot CLI. */
}
