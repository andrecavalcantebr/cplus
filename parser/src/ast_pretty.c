/*
    FILE: ast_pretty.c
    DESCR: Pretty-printer for MPC AST of Cplus (classes, interfaces, sections, fields, methods)
    AUTHOR: Andre Cavalcante
    DATE: August, 2025
    LICENSE: CC BY-SA
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <mpc.h>

/* Forward declarations to avoid implicit warnings (and to control order) */
static void print_type(const mpc_ast_t *type, int level);
static void print_param(const mpc_ast_t *param, int level);
void print_program(const mpc_ast_t *root);

/* =============================================================
 * Small helpers
 * ============================================================= */
static bool has_tag(const mpc_ast_t *n, const char *tag)
{
    return n && n->tag && strstr(n->tag, tag) != NULL;
}

static void collect_text_rec(const mpc_ast_t *n, char *buf, size_t cap)
{
    if (!n || !buf || cap == 0)
        return;
    if (n->children_num == 0)
    {
        if (n->contents && *n->contents)
        {
            size_t blen = strlen(buf);
            size_t nlen = strlen(n->contents);
            if (blen < cap - 1)
            {
                size_t cpy = nlen;
                if (blen + cpy >= cap)
                    cpy = cap - blen - 1;
                memcpy(buf + blen, n->contents, cpy);
                buf[blen + cpy] = '\0';
            }
        }
        return;
    }
    for (int i = 0; i < n->children_num; i++)
    {
        collect_text_rec(n->children[i], buf, cap);
    }
}

/* Concatenate leaf contents and normalize spaces */
static void node_text(const mpc_ast_t *n, char *out, size_t cap)
{
    if (!out || cap == 0)
        return;
    out[0] = '\0';
    collect_text_rec(n, out, cap);
    /* compact spaces */
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
}

static const mpc_ast_t *first_child_tag(const mpc_ast_t *n, const char *tag)
{
    if (!n)
        return NULL;
    for (int i = 0; i < n->children_num; i++)
    {
        if (has_tag(n->children[i], tag))
            return n->children[i];
    }
    return NULL;
}

static void put_indent(int level)
{
    for (int i = 0; i < level; i++)
        fputs("  ", stdout);
}

/* =============================================================
 * Identifier helpers (ignore any subtree tagged as 'type')
 * ============================================================= */

/* DFS that returns an identifier, skipping any subtree tagged as 'type'.
   If rightmost=true, prefer the rightmost identifier; otherwise leftmost. */
static const mpc_ast_t *find_id_excluding_type_dfs(const mpc_ast_t *n, bool rightmost)
{
    if (!n)
        return NULL;
    if (has_tag(n, "type"))
        return NULL; /* do not enter type subtrees */

    /* check self first */
    if (has_tag(n, "identifier"))
        return n;

    int start = rightmost ? n->children_num - 1 : 0;
    int step = rightmost ? -1 : 1;

    for (int i = start; rightmost ? (i >= 0) : (i < n->children_num); i += step)
    {
        const mpc_ast_t *c = n->children[i];
        const mpc_ast_t *id = find_id_excluding_type_dfs(c, rightmost);
        if (id)
            return id;
    }
    return NULL;
}

/* Right-most identifier NOT inside a 'type' subtree. */
static const mpc_ast_t *rightmost_id_outside_type(const mpc_ast_t *m)
{
    return find_id_excluding_type_dfs(m, true);
}

/* First identifier AFTER return 'type' and BEFORE '(' — best for method names */
static const mpc_ast_t *first_id_after_type_until_lparen(const mpc_ast_t *m)
{
    if (!m)
        return NULL;
    bool seen_type = false;
    for (int i = 0; i < m->children_num; i++)
    {
        const mpc_ast_t *c = m->children[i];
        if (!seen_type)
        {
            if (has_tag(c, "type"))
                seen_type = true;
            continue;
        }
        if (has_tag(c, "lparen"))
            break; /* stop before params */
        const mpc_ast_t *id = find_id_excluding_type_dfs(c, false);
        if (id)
            return id;
    }
    return NULL;
}

/* =============================================================
 * Type & Param (definitions before first use)
 * ============================================================= */
static void print_type(const mpc_ast_t *type, int level)
{
    if (!type)
        return;
    char tbuf[256];
    node_text(type, tbuf, sizeof tbuf);
    put_indent(level);
    printf("Type: %s\n", *tbuf ? tbuf : "<unknown>");
}

static void print_param(const mpc_ast_t *param, int level)
{
    if (!param)
        return;
    const mpc_ast_t *ptype = first_child_tag(param, "type");
    /* pick the identifier that is not part of the type */
    const mpc_ast_t *pname = find_id_excluding_type_dfs(param, true);

    put_indent(level);
    printf("Param:\n");
    if (ptype)
        print_type(ptype, level + 1);
    if (pname)
    {
        char nbuf[128] = {0};
        node_text(pname, nbuf, sizeof nbuf);
        put_indent(level + 1);
        printf("Name: %s\n", nbuf);
    }
}

/* =============================================================
 * Param list helpers
 * ============================================================= */

/* Diz se existe pelo menos um 'param' neste nó OU embaixo dele (shape normal ou achatado) */
static bool has_any_param(const mpc_ast_t *plist)
{
    if (!plist)
        return false;

    /* o próprio nó pode ser um 'param' */
    if (has_tag(plist, "param"))
        return true;

    for (int i = 0; i < plist->children_num; i++)
    {
        const mpc_ast_t *c = plist->children[i];
        if (has_tag(c, "param"))
            return true;

        /* caminhar por containers intermediários */
        if (has_tag(c, "param_list") || has_tag(c, "param_list_opt"))
        {
            if (has_any_param(c))
                return true;
        }
    }
    return false;
}

/* Imprime todos os params, aceitando shape container (param_list/param_list_opt) e shape achatado */
static void print_param_list(const mpc_ast_t *plist, int level)
{
    if (!plist)
        return;

    /* shape achatado: o próprio nó já é um 'param' */
    if (has_tag(plist, "param"))
    {
        print_param(plist, level);
        return;
    }

    for (int i = 0; i < plist->children_num; i++)
    {
        const mpc_ast_t *c = plist->children[i];

        if (has_tag(c, "param"))
        {
            print_param(c, level);
        }
        else if (has_tag(c, "param_list") || has_tag(c, "param_list_opt"))
        {
            /* recursivo para atravessar wrappers */
            print_param_list(c, level);
        }
    }
}

/* =============================================================
 * Members: methods and fields
 * ============================================================= */
static void print_method(const mpc_ast_t *m, int level)
{
    if (!m)
        return;
    const mpc_ast_t *ret = first_child_tag(m, "type");
    const mpc_ast_t *name = first_child_tag(m, "method_name");
    const mpc_ast_t *idnod = NULL;

    if (name)
    {
        /* try child identifier; if absent, use the node itself */
        idnod = first_child_tag(name, "identifier");
        if (!idnod)
            idnod = name;
    }
    if (!idnod)
    {
        idnod = first_id_after_type_until_lparen(m);
        if (!idnod)
            idnod = rightmost_id_outside_type(m);
    }

    const mpc_ast_t *plist = first_child_tag(m, "param_list");
    if (!plist)
        plist = first_child_tag(m, "param_list_opt");

    char nbuf[128] = {0};
    if (idnod)
        node_text(idnod, nbuf, sizeof nbuf);

    put_indent(level);
    printf("Method %s\n", *nbuf ? nbuf : "<anon>");

    if (ret)
    {
        char tbuf[256];
        node_text(ret, tbuf, sizeof tbuf);
        put_indent(level + 1);
        printf("Return Type: %s\n", *tbuf ? tbuf : "<unknown>");
    }
    if (plist && has_any_param(plist))
    {
        put_indent(level + 1);
        puts("Params");
        print_param_list(plist, level + 2);
    }
}

static void print_field(const mpc_ast_t *f, int level)
{
    const mpc_ast_t *ft = first_child_tag(f, "type");
    const mpc_ast_t *fid = rightmost_id_outside_type(f);

    char nbuf[128] = {0};
    if (fid)
        node_text(fid, nbuf, sizeof nbuf);

    put_indent(level);
    printf("Field %s\n", *nbuf ? nbuf : "<anon>");
    if (ft)
        print_type(ft, level + 1);
}

static void print_member(const mpc_ast_t *m, int level)
{
    if (!m)
        return;
    if (has_tag(m, "method_decl"))
    {
        print_method(m, level);
        return;
    }
    if (has_tag(m, "field_decl"))
    {
        print_field(m, level);
        return;
    }
    /* heuristics for older grammars */
    if (first_child_tag(m, "method_name") || first_child_tag(m, "lparen"))
    {
        print_method(m, level);
    }
}

/* =============================================================
 * Sections / Interface / Class / Program
 * ============================================================= */
static void print_section(const mpc_ast_t *sec, int level)
{
    if (!sec)
        return;
    const mpc_ast_t *ak = first_child_tag(sec, "access_kw");
    char akbuf[32] = {0};
    if (ak)
        node_text(ak, akbuf, sizeof akbuf);

    put_indent(level);
    printf("Section %s\n", *akbuf ? akbuf : "<access>");

    for (int i = 0; i < sec->children_num; i++)
    {
        const mpc_ast_t *c = sec->children[i];
        if (has_tag(c, "member"))
            print_member(c, level + 1);
    }
}

static void print_interface(const mpc_ast_t *itf, int level)
{
    const mpc_ast_t *id = first_child_tag(itf, "identifier");
    char nbuf[128] = {0};
    if (id)
        node_text(id, nbuf, sizeof nbuf);

    put_indent(level);
    printf("Interface %s\n", *nbuf ? nbuf : "<anon>");

    for (int i = 0; i < itf->children_num; i++)
    {
        const mpc_ast_t *c = itf->children[i];
        if (has_tag(c, "method_decl"))
            print_member(c, level + 1);
        if (has_tag(c, "section"))
            print_section(c, level + 1);
    }
}

static void print_class(const mpc_ast_t *cls, int level)
{
    const mpc_ast_t *id = first_child_tag(cls, "identifier");
    char nbuf[128] = {0};
    if (id)
        node_text(id, nbuf, sizeof nbuf);

    put_indent(level);
    printf("Class %s\n", *nbuf ? nbuf : "<anon>");

    for (int i = 0; i < cls->children_num; i++)
    {
        const mpc_ast_t *c = cls->children[i];
        if (has_tag(c, "section"))
            print_section(c, level + 1);
        else if (has_tag(c, "member"))
            print_member(c, level + 1);
    }
}

void print_program(const mpc_ast_t *root)
{
    if (!root)
        return;
    const mpc_ast_t *prog = root;
    if (!has_tag(prog, "program"))
    {
        const mpc_ast_t *maybe = first_child_tag(root, "program");
        prog = maybe ? maybe : root;
    }

    puts("Program");
    bool printed_any = false;

    for (int i = 0; i < prog->children_num; i++)
    {
        const mpc_ast_t *c = prog->children[i];

        if (has_tag(c, "interface"))
        {
            print_interface(c, 1);
            printed_any = true;
        }
        else if (has_tag(c, "class"))
        {
            print_class(c, 1);
            printed_any = true;
        }
        else if (has_tag(c, "section"))
        {
            print_section(c, 1);
            printed_any = true;
        }
        else if (has_tag(c, "member"))
        {
            print_member(c, 1);
            printed_any = true;
        }
        else
        {
            const mpc_ast_t *itf = first_child_tag(c, "interface");
            const mpc_ast_t *cls = first_child_tag(c, "class");
            if (itf)
            {
                print_interface(itf, 1);
                printed_any = true;
            }
            if (cls)
            {
                print_class(cls, 1);
                printed_any = true;
            }
        }
    }

    if (!printed_any)
    {
        /* Fallback: imprime a AST crua para entendermos a estrutura exata */
        mpc_ast_print((mpc_ast_t *)prog);
    }
}
