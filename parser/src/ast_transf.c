/*
    FILE: ast_transf.c
    DESCRIPTION: Simple AST -> C code generator
    - Transforms 'class' declarations into C 'struct' + free prototypes
    - Echoes other C code unchanged
*/

#include "ast.h"
#include <stdio.h>
#include <string.h>

static int has_tag(const mpc_ast_t *n, const char *tag)
{
    return n && n->tag && strstr(n->tag, tag) != NULL;
}

static void print_text_rec(const mpc_ast_t *n)
{
    if (!n)
        return;
    if (n->children_num == 0)
    {
        if (n->contents && *n->contents)
            fputs(n->contents, stdout);
        return;
    }
    for (int i = 0; i < n->children_num; i++)
        print_text_rec(n->children[i]);
}

static const mpc_ast_t *find_first(const mpc_ast_t *n, const char *tag)
{
    if (!n)
        return NULL;
    if (has_tag(n, tag))
        return n;
    for (int i = 0; i < n->children_num; i++)
    {
        const mpc_ast_t *r = find_first(n->children[i], tag);
        if (r)
            return r;
    }
    return NULL;
}

static void emit_class(const mpc_ast_t *class_decl)
{
    const mpc_ast_t *hdr = find_first(class_decl, "class_header");
    const mpc_ast_t *idn = find_first(hdr, "ident");
    const char *name = (idn && idn->contents && *idn->contents) ? idn->contents : "Anon";

    /* collect full class text */
    char buf[8192] = {0};
    /* recursive appender */
    typedef struct
    {
        char *b;
        size_t n;
        size_t cap;
    } SB;
    void append(const char *s, SB *sb)
    {
        if (!s)
            return;
        size_t m = strlen(s);
        if (sb->n + m >= sb->cap)
            m = (sb->cap ? sb->cap - 1 - sb->n : 0);
        if (m > 0)
        {
            memcpy(sb->b + sb->n, s, m);
            sb->n += m;
            sb->b[sb->n] = 0;
        }
    }
    void append_rec(const mpc_ast_t *node, SB *sb)
    {
        if (!node)
            return;
        if (node->children_num == 0)
        {
            if (node->contents && *node->contents)
                append(node->contents, sb);
            return;
        }
        for (int k = 0; k < node->children_num; k++)
            append_rec(node->children[k], sb);
    }
    SB sb = {buf, 0, sizeof buf};
    append_rec(class_decl, &sb);

    /* find body between first '{' and last '}' */
    const char *all = buf;
    const char *lb = strchr(all, '{');
    const char *rb = strrchr(all, '}');
    const char *endsemi = strrchr(all, ';');
    if (!lb || !rb || !endsemi || endsemi < rb)
        return;
    const char *body = lb + 1;
    size_t blen = (size_t)(rb - body);
    /* suffix declarators between '}' and final ';' (if any) */
    const char *suf_start = rb + 1;
    const char *suf_end = endsemi; /* points to ';' */

    /* emit typedef so 'Name *' is a valid C type */
    fputs("typedef struct ", stdout);
    fputs(name, stdout);
    fputc(' ', stdout);
    fputs(name, stdout);
    fputs(";\n", stdout);

    fputs("typedef ", stdout);
    fputs(name, stdout);
    fputc(' ', stdout);
    fputs(name, stdout);
    fputs("_ref[1];\n", stdout);

    /* print struct */
    fputs("struct ", stdout);
    fputs(name, stdout);
    fputs(" {\n", stdout);

    /* split body by ';' and classify */
    const char *p = body;
    while (p < body + blen)
    {
        const char *ssemi = memchr(p, ';', (size_t)((body + blen) - p));
        if (!ssemi)
            break;
        size_t len = (size_t)(ssemi - p);
        /* trim leading/trailing spaces */
        while (len && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
        {
            p++;
            len--;
        }
        while (len && (p[len - 1] == ' ' || p[len - 1] == '\t' || p[len - 1] == '\n' || p[len - 1] == '\r'))
        {
            len--;
        }
        if (len)
        {
            int is_method = 0;
            for (size_t i = 0; i < len; i++)
            {
                if (p[i] == '(')
                {
                    is_method = 1;
                    break;
                }
            }
            if (!is_method)
            {
                /* strip optional access modifier */
                const char *q = p;
                while (q < p + len && (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r'))
                    q++;
                const char *after = q;
                if ((size_t)(p + len - q) >= 6 && strncmp(q, "public", 6) == 0)
                {
                    after = q + 6;
                    while (after < p + len && (*after == ' ' || *after == '\t'))
                        after++;
                }
                else if ((size_t)(p + len - q) >= 7 && strncmp(q, "private", 7) == 0)
                {
                    after = q + 7;
                    while (after < p + len && (*after == ' ' || *after == '\t'))
                        after++;
                }
                fputs("    ", stdout);
                fwrite(after, 1, (size_t)((p + len) - after), stdout);
                fputs(";\n", stdout);
            }
        }
        p = ssemi + 1;
    }
    fputs("};\n", stdout);

    /* emit trailing declarators as 'struct Name <suffix>;' if any */
    if (suf_start && suf_end && suf_end > suf_start)
    {
        /* trim spaces */
        while (suf_start < suf_end && (*suf_start == ' ' || *suf_start == '\t' || *suf_start == '\n' || *suf_start == '\r'))
            suf_start++;
        const char *suf_last = suf_end;
        while (suf_last > suf_start && (suf_last[-1] == ' ' || suf_last[-1] == '\t' || suf_last[-1] == '\n' || suf_last[-1] == '\r'))
            suf_last--;
        if (suf_last > suf_start)
        {
            fputs("struct ", stdout);
            fputs(name, stdout);
            fputc(' ', stdout);
            fwrite(suf_start, 1, (size_t)(suf_last - suf_start), stdout);
            fputs(";\n", stdout);
        }
    }

    /* emit method prototypes after (rename to Class_Method) */
    p = body;
    while (p < body + blen)
    {
        const char *ssemi = memchr(p, ';', (size_t)((body + blen) - p));
        if (!ssemi)
            break;
        size_t len = (size_t)(ssemi - p);
        /* trim spaces */
        while (len && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
        {
            p++;
            len--;
        }
        while (len && (p[len - 1] == ' ' || p[len - 1] == '\t' || p[len - 1] == '\n' || p[len - 1] == '\r'))
        {
            len--;
        }
        if (len)
        {
            /* classify */
            const char *lpar = memchr(p, '(', len);
            if (lpar)
            {
                /* find method name: scan left from lpar to previous ident */
                const char *name_end = lpar;
                const char *q = name_end;
                while (q > p && (q[-1] == ' ' || q[-1] == '\t'))
                    q--;
                const char *name_start = q;
                while (name_start > p && ((name_start[-1] == '_') || (name_start[-1] >= '0' && name_start[-1] <= '9') || (name_start[-1] >= 'A' && name_start[-1] <= 'Z') || (name_start[-1] >= 'a' && name_start[-1] <= 'z')))
                    name_start--;
                /* If there's a leading access modifier before the return type, skip it in the return part */
                const char *ret_start = p;
                while (ret_start < name_start && (*ret_start == ' ' || *ret_start == '\t' || *ret_start == '\n' || *ret_start == '\r'))
                    ret_start++;
                if ((size_t)(name_start - ret_start) >= 6 && strncmp(ret_start, "public", 6) == 0)
                {
                    ret_start += 6;
                    while (ret_start < name_start && (*ret_start == ' ' || *ret_start == '\t'))
                        ret_start++;
                }
                else if ((size_t)(name_start - ret_start) >= 7 && strncmp(ret_start, "private", 7) == 0)
                {
                    ret_start += 7;
                    while (ret_start < name_start && (*ret_start == ' ' || *ret_start == '\t'))
                        ret_start++;
                }
                /* print return part */
                fwrite(ret_start, 1, (size_t)(name_start - ret_start), stdout);
                /* Class_Method */
                fputs(name, stdout);
                fputc('_', stdout);
                fwrite(name_start, 1, (size_t)(name_end - name_start), stdout);
                /* print params and close ; */
                fwrite(lpar, 1, (size_t)(len - (lpar - p)), stdout);
                fputs(";\n", stdout);
            }
        }
        p = ssemi + 1;
    }
}

static void walk_and_emit(const mpc_ast_t *n)
{
    if (!n)
        return;
    if (has_tag(n, "class_decl"))
    {
        emit_class(n);
        return;
    }
    if (has_tag(n, "top_item") || has_tag(n, "c_decl") || has_tag(n, "c_funcdef") || has_tag(n, "pp_line") || has_tag(n, "stmt"))
    {
        print_text_rec(n);
        fputc('\n', stdout);
        return;
    }
    for (int i = 0; i < n->children_num; i++)
        walk_and_emit(n->children[i]);
}

void ast_transformation(mpc_ast_t *ast, const char *output_dir)
{
    (void)output_dir;
    walk_and_emit(ast);
}
