/*
    FILE: ast_transf.c
    DESCR: Simple AST → C code generator for Cplus
    AUTHOR: Andre Cavalcante
    DATE: October, 2025
    LICENSE: CC BY-SA
*/

#include "ast.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*
 * Helper to check if a node has a given tag substring.
 */
static int has_tag(const mpc_ast_t *n, const char *tag)
{
    return n && n->tag && strstr(n->tag, tag) != NULL;
}

/*
 * Recursively find the first AST node with a tag containing the given
 * substring. Returns NULL if none is found.
 */
static const mpc_ast_t *find_first(const mpc_ast_t *n, const char *tag)
{
    if (!n)
    {
        return NULL;
    }
    if (has_tag(n, tag))
    {
        return n;
    }
    for (int i = 0; i < n->children_num; i++)
    {
        const mpc_ast_t *r = find_first(n->children[i], tag);
        if (r)
        {
            return r;
        }
    }
    return NULL;
}

/*
 * Collect the contents of an AST subtree into a buffer, concatenating
 * all child text. The buffer will contain the textual representation
 * of the subtree.
 */
static void collect_text_rec(const mpc_ast_t *n, char *buf, size_t *len, size_t max)
{
    if (!n || *len >= max - 1)
    {
        return;
    }
    if (n->children_num == 0)
    {
        if (n->contents && *n->contents)
        {
            size_t m = strlen(n->contents);
            if (*len + m >= max - 1)
            {
                m = max - 1 - *len;
            }
            if (m > 0)
            {
                memcpy(buf + *len, n->contents, m);
                *len += m;
                buf[*len] = '\0';
            }
        }
        return;
    }
    for (int i = 0; i < n->children_num; i++)
    {
        collect_text_rec(n->children[i], buf, len, max);
    }
}

/*
 * Print the contents of an AST subtree exactly as it appears in the
 * input source, preserving whitespace.
 */
static void print_text_rec(const mpc_ast_t *n)
{
    if (!n)
    {
        return;
    }
    if (n->children_num == 0)
    {
        if (n->contents && *n->contents)
        {
            fputs(n->contents, stdout);
        }
        return;
    }
    for (int i = 0; i < n->children_num; i++)
    {
        print_text_rec(n->children[i]);
    }
}

/*
 * Simple registry of methods declared inside classes. It can be used to
 * perform additional checks, but the current warning logic derives the
 * class name from the function definition directly.
 */
typedef struct
{
    char class_name[64];
    char method_name[64];
} MethodEntry;

static MethodEntry method_map[256];
static int method_count = 0;

static void add_method(const char *class_name, const char *method)
{
    if (method_count >= (int)(sizeof method_map / sizeof method_map[0]))
    {
        return;
    }
    strncpy(method_map[method_count].class_name, class_name,
            sizeof(method_map[method_count].class_name) - 1);
    method_map[method_count].class_name[sizeof(method_map[0].class_name) - 1] = '\0';
    strncpy(method_map[method_count].method_name, method,
            sizeof(method_map[method_count].method_name) - 1);
    method_map[method_count].method_name[sizeof(method_map[0].method_name) - 1] = '\0';
    method_count++;
}

/*
 * Determine whether a token corresponds to a C keyword or one of our
 * access modifiers. Used when extracting method names to avoid
 * registering keywords as method names.
 */
static int is_c_keyword(const char *tok)
{
    return !strcmp(tok, "void") || !strcmp(tok, "int") ||
           !strcmp(tok, "char") || !strcmp(tok, "float") ||
           !strcmp(tok, "double") || !strcmp(tok, "short") ||
           !strcmp(tok, "long") || !strcmp(tok, "signed") ||
           !strcmp(tok, "unsigned") || !strcmp(tok, "static") ||
           !strcmp(tok, "public") || !strcmp(tok, "private");
}

/*
 * Extract the identifier immediately preceding an opening parenthesis.
 * Returns 1 if an identifier was found and written to `out`, 0 otherwise.
 */
static int ident_before_paren(const char *seg, size_t seglen,
                              char *out, size_t outsz)
{
    const char *lpar = memchr(seg, '(', seglen);
    if (!lpar)
    {
        return 0;
    }
    /* Move left past whitespace to the end of the identifier. */
    const char *end = lpar;
    while (end > seg && isspace((unsigned char)end[-1]))
    {
        end--;
    }
    /* Move left over valid identifier characters. */
    const char *start = end;
    while (start > seg && (isalnum((unsigned char)start[-1]) || start[-1] == '_'))
    {
        start--;
    }
    size_t n = (size_t)(end - start);
    if (n == 0 || n >= outsz)
    {
        return 0;
    }
    memcpy(out, start, n);
    out[n] = '\0';
    return 1;
}

/*
 * Attempt to extract the class name from the first parameter of a
 * function definition. The expected pattern is "<Class>_ref", where
 * <Class> is an identifier. Returns 1 on success and writes the class
 * name into `out`.
 */
static int extract_class_from_first_param(const char *p,
                                          char *out, size_t outsz)
{
    /* Skip leading whitespace. */
    while (*p && isspace((unsigned char)*p))
    {
        p++;
    }
    /* Capture a single token up to whitespace, comma or closing paren. */
    const char *beg = p;
    while (*p && !isspace((unsigned char)*p) && *p != ',' && *p != ')')
    {
        p++;
    }
    size_t tok_len = (size_t)(p - beg);
    /* Find the substring "_ref" at the end of the token. */
    if (tok_len < 4)
    {
        return 0;
    }
    if (strncmp(beg + tok_len - 4, "_ref", 4) != 0)
    {
        return 0;
    }
    size_t clslen = tok_len - 4;
    if (clslen == 0 || clslen >= outsz)
    {
        return 0;
    }
    memcpy(out, beg, clslen);
    out[clslen] = '\0';
    return 1;
}

/*
 * Emit C code for a class declaration. This prints typedefs for the
 * class and class_ref types, then prints the struct with its fields
 * (omitting access modifiers). Afterwards it emits renamed method
 * prototypes (e.g. Foo_get), registering the method names for
 * possible future use.
 */
static void emit_class(const mpc_ast_t *class_decl)
{
    /* Locate the class name using our find_first helper. */
    const mpc_ast_t *hdr = find_first(class_decl, "class_header");
    const mpc_ast_t *idn = find_first(hdr, "ident");
    const char *name = (idn && idn->contents && *idn->contents)
                           ? idn->contents
                           : "Anon";

    /* Collect the raw text of the entire class declaration. */
    char buf[8192] = {0};
    size_t buflen = 0;
    collect_text_rec(class_decl, buf, &buflen, sizeof buf);

    /* Find the class body between the first '{' and the last '}' */
    const char *lb = strchr(buf, '{');
    const char *rb = strrchr(buf, '}');
    const char *endsemi = strrchr(buf, ';');
    if (!lb || !rb || !endsemi || endsemi < rb)
    {
        return;
    }

    /* Emit the typedef for the struct and its reference type. */
    printf("typedef struct %s %s;\n", name, name);
    printf("typedef %s %s_ref[1];\n", name, name);

    /* Begin the struct definition. */
    printf("struct %s {\n", name);

    /* Process each declaration inside the class body. */
    const char *body = lb + 1;
    size_t blen = (size_t)(rb - body);
    const char *p = body;
    while (p < body + blen)
    {
        const char *ssemi = memchr(p, ';', (size_t)((body + blen) - p));
        if (!ssemi)
        {
            break;
        }
        size_t seglen = (size_t)(ssemi - p);
        /* Skip leading whitespace. */
        while (seglen && isspace((unsigned char)*p))
        {
            p++;
            seglen--;
        }
        /* Trim trailing whitespace. */
        while (seglen &&
               isspace((unsigned char)p[seglen - 1]))
        {
            seglen--;
        }
        if (seglen)
        {
            /* Determine if this segment is a method prototype. */
            int is_method = 0;
            for (size_t i = 0; i < seglen; i++)
            {
                if (p[i] == '(')
                {
                    is_method = 1;
                    break;
                }
            }
            if (!is_method)
            {
                /* Field declaration: drop 'public'/'private' and print. */
                const char *q = p;
                size_t qlen = seglen;
                /* Strip leading keywords. */
                if (qlen >= 6 && strncmp(q, "public", 6) == 0)
                {
                    q += 6;
                    qlen -= 6;
                    while (qlen && isspace((unsigned char)*q))
                    {
                        q++;
                        qlen--;
                    }
                }
                else if (qlen >= 7 && strncmp(q, "private", 7) == 0)
                {
                    q += 7;
                    qlen -= 7;
                    while (qlen && isspace((unsigned char)*q))
                    {
                        q++;
                        qlen--;
                    }
                }
                /* Print field with indentation and semicolon. */
                printf("    %.*s;\n", (int)qlen, q);
            }
            else
            {
                /* Method prototype: rename and register. */
                /* Register the method name if not a keyword. */
                char mname[64];
                if (ident_before_paren(p, seglen, mname, sizeof mname))
                {
                    if (!is_c_keyword(mname))
                    {
                        add_method(name, mname);
                    }
                }
                /* Determine parts for printing. */
                const char *lpar = memchr(p, '(', seglen);
                const char *name_end = lpar;
                const char *q = name_end;
                /* Move q left past whitespace. */
                while (q > p && isspace((unsigned char)q[-1]))
                {
                    q--;
                }
                const char *name_start = q;
                /* Move name_start left over identifier characters. */
                while (name_start > p &&
                       (isalnum((unsigned char)name_start[-1]) || name_start[-1] == '_'))
                {
                    name_start--;
                }
                /* Determine return type range (p to name_start). */
                const char *retp = p;
                size_t retl = (size_t)(name_start - retp);
                /* Strip 'public'/'private' from the return type part. */
                /* Trim leading whitespace. */
                while (retl && isspace((unsigned char)*retp))
                {
                    retp++;
                    retl--;
                }
                if (retl >= 6 && strncmp(retp, "public", 6) == 0)
                {
                    retp += 6;
                    retl -= 6;
                    while (retl && isspace((unsigned char)*retp))
                    {
                        retp++;
                        retl--;
                    }
                }
                else if (retl >= 7 && strncmp(retp, "private", 7) == 0)
                {
                    retp += 7;
                    retl -= 7;
                    while (retl && isspace((unsigned char)*retp))
                    {
                        retp++;
                        retl--;
                    }
                }
                /* Print return type. */
                if (retl > 0)
                {
                    fwrite(retp, 1, retl, stdout);
                    /* Ensure there is exactly one space before the method name. */
                    if (!isspace((unsigned char)retp[retl - 1]))
                    {
                        fputc(' ', stdout);
                    }
                }
                /* Print renamed method: Class_Method */
                printf("%s_", name);
                fwrite(name_start, 1, (size_t)(name_end - name_start), stdout);
                /* Print the remainder of the prototype from '(' to end. */
                fwrite(lpar, 1, (size_t)(seglen - (lpar - p)), stdout);
                /* Add terminating semicolon and newline. */
                fputs(";\n", stdout);
            }
        }
        /* Advance past the semicolon. */
        p = ssemi + 1;
    }
    /* Close the struct. */
    printf("};\n");
}

/*
 * Emit a function definition. It also checks if the function appears to
 * operate on a class instance (its first parameter is <Class>_ref).
 * If so, and if the function name does not start with "<Class>_", a
 * warning is printed to both stderr and stdout.
 */
static void emit_func_with_warning(const mpc_ast_t *func)
{
    /* Collect the original function text. */
    char buf[8192] = {0};
    size_t len = 0;
    collect_text_rec(func, buf, &len, sizeof buf);

    /* Find the opening parenthesis. */
    const char *lpar = strchr(buf, '(');
    if (!lpar)
    {
        fputs(buf, stdout);
        fputc('\n', stdout);
        return;
    }
    /* Extract the function name (identifier before '('). */
    const char *name_end = lpar;
    const char *q = name_end;
    while (q > buf && isspace((unsigned char)q[-1]))
    {
        q--;
    }
    const char *name_start = q;
    while (name_start > buf &&
           (isalnum((unsigned char)name_start[-1]) || name_start[-1] == '_'))
    {
        name_start--;
    }
    size_t fnlen = (size_t)(name_end - name_start);
    char fname[128] = {0};
    if (fnlen > 0 && fnlen < sizeof fname)
    {
        memcpy(fname, name_start, fnlen);
        fname[fnlen] = '\0';
    }

    /* Derive class name from first parameter if possible. */
    char clsname[128];
    if (extract_class_from_first_param(lpar + 1, clsname, sizeof clsname))
    {
        size_t clslen = strlen(clsname);
        /* Check if function name already starts with Class_ */
        int has_prefix = (fnlen > clslen &&
                          strncmp(fname, clsname, clslen) == 0 &&
                          fname[clslen] == '_');
        if (!has_prefix)
        {
            /* Emit warning to stderr and stdout. */
            fprintf(stderr,
                    "Cplus warning: function '%s' has first parameter of type '%s_ref self' — "
                    "did you mean '%s_%s(...)'?\n",
                    fname, clsname, clsname, fname);
            fflush(stderr);
        }
    }

    /* Emit the function definition unchanged. */
    fputs(buf, stdout);
    fputc('\n', stdout);
}

static int looks_like_funcdef(const mpc_ast_t *n)
{
    /* Heurística textual: tem '(' e ')' antes do primeiro '{', e não há ';' antes do '{' */
    char buf[2048] = {0};
    size_t len = 0;
    collect_text_rec(n, buf, &len, sizeof buf);

    const char *lb = strchr(buf, '{');
    if (!lb)
        return 0;
    const char *lp = strchr(buf, '(');
    const char *rp = lp ? strchr(lp, ')') : NULL;
    const char *sc = strchr(buf, ';');

    /* deve existir '(' e ')' antes de '{' e não pode haver ';' antes de '{' */
    if (!lp || !rp)
        return 0;
    if (lp > lb || rp > lb)
        return 0;
    if (sc && sc < lb)
        return 0;
    return 1;
}

static void walk_and_emit(const mpc_ast_t *n)
{
    if (!n)
        return;

    /* classes primeiro */
    if (has_tag(n, "class_decl"))
    {
        emit_class(n);
        return;
    }

    /* várias possibilidades de tags de funcdef */
    if (has_tag(n, "c_funcdef") ||
        has_tag(n, "funcdef") ||
        has_tag(n, "c_funcdef_raw") ||
        has_tag(n, "c_funcdef_two_ident"))
    {
        emit_func_with_warning(n);
        return;
    }

    /* fallback: top_item que parece definição de função */
    if (has_tag(n, "top_item"))
    {
        if (looks_like_funcdef(n))
        {
            emit_func_with_warning(n);
            return;
        }
        print_text_rec(n);
        fputc('\n', stdout);
        return;
    }

    /* demais nós "simples" */
    if (has_tag(n, "c_decl") || has_tag(n, "pp_line") || has_tag(n, "stmt"))
    {
        print_text_rec(n);
        fputc('\n', stdout);
        return;
    }

    /* recursão padrão */
    for (int i = 0; i < n->children_num; i++)
        walk_and_emit(n->children[i]);
}

/*
 * Entry point for AST transformation. The output_dir parameter is
 * ignored; output is written to stdout.
 */
void ast_transformation(mpc_ast_t *ast, const char *output_dir)
{
    (void)output_dir;
    walk_and_emit(ast);
}
