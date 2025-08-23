/*
    FILE: main.c
    DESCR: Code generator for a self-contained Cplus parser (MPC/mpca_lang)
           Reads a grammar.mpc and emits a C program that embeds the grammar
           and supports: -G (print grammar), -f <file> (parse file),
                         -x "<expr>" (parse string)
           The generated program:
             - creates all mpc_parser_t* with mpc_new("<rule>")
             - calls mpca_lang(..., <rules>..., NULL) in the same order
             - parses input using the 'program' rule
             - prints AST using print_program() and mpc_ast_print()
             - cleans up with mpc_cleanup(N, <rules>...)
    AUTHOR: Andre Cavalcante
    DATE: August, 2025
    LICENSE: CC BY-SA
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

/* ============================================================
 * Utility: read entire file (binary-safe)
 * ============================================================ */
static char *slurp(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        fprintf(stderr, "Error opening file: %s (%s)\n", path, strerror(errno));
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "Error seeking file: %s\n", path);
        fclose(f);
        return NULL;
    }
    long n = ftell(f);
    if (n < 0)
    {
        fprintf(stderr, "Error telling file size: %s\n", path);
        fclose(f);
        return NULL;
    }
    rewind(f);

    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf)
    {
        fprintf(stderr, "Out of memory reading: %s\n", path);
        fclose(f);
        return NULL;
    }
    size_t rd = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[rd] = '\0';
    if (out_len)
        *out_len = rd;
    return buf;
}

/* ============================================================
 * Helpers: dynamic array of strings (rule names)
 * ============================================================ */
typedef struct
{
    char **v;
    size_t n, cap;
} StrVec;

static void sv_init(StrVec *sv)
{
    sv->v = NULL;
    sv->n = 0;
    sv->cap = 0;
}

static int sv_contains(const StrVec *sv, const char *s)
{
    for (size_t i = 0; i < sv->n; ++i)
        if (strcmp(sv->v[i], s) == 0)
            return 1;
    return 0;
}

static void sv_push(StrVec *sv, const char *s)
{
    if (sv_contains(sv, s))
        return;
    if (sv->n == sv->cap)
    {
        size_t nc = sv->cap ? sv->cap * 2 : 16;
        char **nv = (char **)realloc(sv->v, nc * sizeof(char *));
        if (!nv)
        {
            fprintf(stderr, "Out of memory\n");
            exit(1);
        }
        sv->v = nv;
        sv->cap = nc;
    }
    sv->v[sv->n++] = strdup(s);
}

static void sv_free(StrVec *sv)
{
    for (size_t i = 0; i < sv->n; ++i)
        free(sv->v[i]);
    free(sv->v);
}

/* ============================================================
 * Grammar parsing: collect nonterminals (rule names) in order
 * Rule format assumed: <rule> : ... ;
 * We find the first ':' outside comments/strings/regex and
 * take the token immediately preceding it, trimming spaces.
 * ============================================================ */
static int is_ident_start(int c) { return c == '_' || isalpha(c); }
static int is_ident(int c) { return c == '_' || isalnum(c); }

static void collect_rules(const char *g, size_t glen, StrVec *rules)
{
    enum
    {
        NORM,
        LINE_COMMENT,
        BLOCK_COMMENT,
        SQ,
        DQ,
        REGEX
    } st = NORM;
    int esc = 0;
    size_t i = 0;

    while (i < glen)
    {
        char c = g[i];

        switch (st)
        {
        case NORM:
            if (c == '/')
            {
                if (i + 1 < glen && g[i + 1] == '/')
                {
                    st = LINE_COMMENT;
                    i += 2;
                    break;
                }
                if (i + 1 < glen && g[i + 1] == '*')
                {
                    st = BLOCK_COMMENT;
                    i += 2;
                    break;
                }
                st = REGEX;
                i++;
                break;
            }
            if (c == '\'')
            {
                st = SQ;
                i++;
                esc = 0;
                continue;
            }
            if (c == '\"')
            {
                st = DQ;
                i++;
                esc = 0;
                continue;
            }
            if (c == ':')
            {
                /* backtrack to find the identifier before ':' */
                size_t j = i;
                /* skip spaces left */
                while (j > 0 && isspace((unsigned char)g[j - 1]))
                    j--;
                /* now find start of identifier */
                size_t k = j;
                while (k > 0 && is_ident((unsigned char)g[k - 1]))
                    k--;
                if (k < j && is_ident_start((unsigned char)g[k]))
                {
                    size_t len = j - k;
                    char *name = (char *)malloc(len + 1);
                    memcpy(name, g + k, len);
                    name[len] = '\0';
                    sv_push(rules, name);
                    free(name);
                }
                i++; /* consume ':' */
                continue;
            }
            /* track possible identifiers, not strictly needed here */
            i++;
            break;

        case LINE_COMMENT:
            if (c == '\n')
                st = NORM;
            i++;
            break;

        case BLOCK_COMMENT:
            if (c == '*' && i + 1 < glen && g[i + 1] == '/')
            {
                st = NORM;
                i += 2;
                continue;
            }
            i++;
            break;

        case SQ: /* single-quoted char literal */
            if (!esc && c == '\\')
            {
                esc = 1;
                i++;
                break;
            }
            if (!esc && c == '\'')
            {
                st = NORM;
                i++;
                break;
            }
            esc = 0;
            i++;
            break;

        case DQ: /* double-quoted string literal */
            if (!esc && c == '\\')
            {
                esc = 1;
                i++;
                break;
            }
            if (!esc && c == '\"')
            {
                st = NORM;
                i++;
                break;
            }
            esc = 0;
            i++;
            break;

        case REGEX: /* /regex/ */
            if (!esc && c == '\\')
            {
                esc = 1;
                i++;
                break;
            }
            if (!esc && c == '/')
            {
                st = NORM;
                i++;
                break;
            }
            esc = 0;
            i++;
            break;
        }
    }
}

/* ============================================================
 * Emit C string literal lines for GRAMMAR
 * Fix: close quotes exactly once; if the last line didn't end
 * with newline, close the open quote at the end.
 * ============================================================ */
static void emit_c_string_literal_lines(FILE *out, const char *src, size_t len)
{
    fputs("static const char *GRAMMAR =\n", out);

    int open = 0;
    for (size_t i = 0; i < len; ++i)
    {
        unsigned char c = (unsigned char)src[i];
        if (!open)
        {
            fputs("  \"", out);
            open = 1;
        }

        switch (c)
        {
        case '\\':
            fputs("\\\\", out);
            break;
        case '\"':
            fputs("\\\"", out);
            break;
        case '\t':
            fputs("\\t", out);
            break;
        case '\r':
            fputs("\\r", out);
            break;
        case '\n':
            fputs("\\n\"\n", out);
            open = 0;
            break;
        default:
            if (c < 32 || c == 127)
                fprintf(out, "\\%03o", c);
            else
                fputc(c, out);
            break;
        }
    }
    if (open)
        fputs("\"\n", out); /* close the last opened quote */
    fputs(";\n\n", out);    /* ONLY '";\n\n' as requested */
}

/* ============================================================
 * Emit the generated parser program to stdout.
 * The program embeds:
 *  - GRAMMAR
 *  - Rules array: mpc_parser_t* <rule> = mpc_new("<rule>");
 *  - CLI: -G / -f / -x
 *  - Runtime build via mpca_lang(MPCA_LANG_DEFAULT, GRAMMAR, <rules>..., NULL)
 *  - AST printing: print_program() and mpc_ast_print()
 *  - Cleanup: mpc_cleanup(N, <rules>...)
 * ============================================================ */
static void emit_parser_program(FILE *out, const char *grammar_src, size_t grammar_len, const StrVec *rules)
{
    /* Header and includes */
    fputs(
        "/*\n"
        "    FILE: parser_generated.c\n"
        "    DESCR: Self-contained Cplus parser (MPC/mpca_lang) with CLI\n"
        "            -G : print grammar\n"
        "            -f : parse file\n"
        "            -x : parse string\n"
        "    AUTHOR: Generated by gen_parser\n"
        "    DATE: August, 2025\n"
        "    LICENSE: CC BY-SA\n"
        "*/\n"
        "\n"
        "#define _GNU_SOURCE\n"
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "#include <errno.h>\n"
        "#include <mpc.h>\n"
        "#include \"ast.h\"\n"
        "\n"
        "/* ============== Utility: read entire file ============== */\n"
        "static char *slurp(const char *path, size_t *out_len) {\n"
        "    FILE *f = fopen(path, \"rb\");\n"
        "    if (!f) { fprintf(stderr, \"Error opening file: %s (%s)\\n\", path, strerror(errno)); return NULL; }\n"
        "    if (fseek(f, 0, SEEK_END) != 0) { fprintf(stderr, \"Error seeking file: %s\\n\", path); fclose(f); return NULL; }\n"
        "    long n = ftell(f);\n"
        "    if (n < 0) { fprintf(stderr, \"Error telling file size: %s\\n\", path); fclose(f); return NULL; }\n"
        "    rewind(f);\n"
        "    char *buf = (char *)malloc((size_t)n + 1);\n"
        "    if (!buf) { fprintf(stderr, \"Out of memory reading: %s\\n\", path); fclose(f); return NULL; }\n"
        "    size_t rd = fread(buf, 1, (size_t)n, f);\n"
        "    fclose(f);\n"
        "    buf[rd] = '\\0';\n"
        "    if (out_len) *out_len = rd;\n"
        "    return buf;\n"
        "}\n"
        "\n",
        out);

    /* Embedded grammar string */
    emit_c_string_literal_lines(out, grammar_src, grammar_len);

    /* Declare all parser pointers */
    fputs("/* ============== Parser rules ============== */\n", out);
    for (size_t i = 0; i < rules->n; ++i)
    {
        fprintf(out, "static mpc_parser_t *%s;\n", rules->v[i]);
    }
    fputs("\n", out);

    /* usage() */
    fputs(
        "static void usage(const char *argv0) {\n"
        "    fprintf(stderr,\n"
        "        \"Usage:\\n\"\n"
        "        \"  %s -G\\n\"\n"
        "        \"  %s -f <input.cplus[.h]>\\n\"\n"
        "        \"  %s -x \\\"<source>\\\"\\n\"\n"
        "        \"\\n\"\n"
        "        \"Options:\\n\"\n"
        "        \"  -G          Print the embedded grammar and exit\\n\"\n"
        "        \"  -f <path>   Parse the given Cplus header/source file\\n\"\n"
        "        \"  -x <text>   Parse the given text directly\\n\",\n"
        "        argv0, argv0, argv0);\n"
        "}\n\n",
        out);

    /* build_all_parsers(): mpc_new for each, mpca_lang with ...rules..., NULL */
    fputs(
        "static int build_all_parsers(void) {\n", out);
    for (size_t i = 0; i < rules->n; ++i)
    {
        fprintf(out, "    %s = mpc_new(\"%s\");\n", rules->v[i], rules->v[i]);
    }
    fputs("    /* mpca_lang returns mpc_err_t* (NULL on success) */\n", out);
    fputs("    mpc_err_t *err = mpca_lang(MPCA_LANG_DEFAULT, GRAMMAR,\n", out);
    for (size_t i = 0; i < rules->n; ++i)
    {
        fprintf(out, "        %s,\n", rules->v[i]);
    }
    fputs("        NULL);\n", out);
    fputs("    if (err) { mpc_err_print(err); mpc_err_delete(err); return 0; }\n", out);
    fputs("    return 1;\n}\n\n", out);

    /* cleanup_all_parsers(): mpc_cleanup(N, ...) */
    fputs("static void cleanup_all_parsers(void) {\n", out);
    fprintf(out, "    mpc_cleanup(%zu", rules->n);
    for (size_t i = 0; i < rules->n; ++i)
    {
        fprintf(out, ", %s\n", rules->v[i]);
    }
    fputs(");\n}\n\n", out);

    /* parse_source(): uses 'program' rule explicitly */
    fputs(
        "static int parse_source(const char *input_name, const char *source) {\n"
        "    mpc_result_t r;\n"
        "    if (mpc_parse(input_name, source, program, &r)) {\n"
        "        puts(\"== PARSE SUCCESS ==\");\n"
        "        // print_program(r.output);\n"
        "        mpc_ast_print(r.output);\n"
        "        mpc_ast_delete(r.output);\n"
        "        return 0;\n"
        "    } else {\n"
        "        fprintf(stderr, \"== PARSE ERROR ==\\n\");\n"
        "        mpc_err_print(r.error);\n"
        "        mpc_err_delete(r.error);\n"
        "        return 1;\n"
        "    }\n"
        "}\n\n",
        out);

    /* main() */
    fputs(
        "int main(int argc, char **argv) {\n"
        "    const char *file_path = NULL;\n"
        "    const char *expr_text = NULL;\n"
        "    int print_grammar = 0;\n"
        "\n"
        "    for (int i = 1; i < argc; ++i) {\n"
        "        if (strcmp(argv[i], \"-G\") == 0) {\n"
        "            print_grammar = 1;\n"
        "        } else if (strcmp(argv[i], \"-f\") == 0) {\n"
        "            if (i + 1 >= argc) { usage(argv[0]); return 2; }\n"
        "            file_path = argv[++i];\n"
        "        } else if (strcmp(argv[i], \"-x\") == 0) {\n"
        "            if (i + 1 >= argc) { usage(argv[0]); return 2; }\n"
        "            expr_text = argv[++i];\n"
        "        } else {\n"
        "            usage(argv[0]);\n"
        "            return 2;\n"
        "        }\n"
        "    }\n"
        "\n"
        "    if (print_grammar + (file_path != NULL) + (expr_text != NULL) != 1) {\n"
        "        usage(argv[0]);\n"
        "        return 2;\n"
        "    }\n"
        "\n"
        "    if (print_grammar) { fputs(GRAMMAR, stdout); return 0; }\n"
        "\n"
        "    if (!build_all_parsers()) { cleanup_all_parsers(); return 4; }\n"
        "\n"
        "    int rc = 0;\n"
        "    if (file_path) {\n"
        "        size_t src_len = 0;\n"
        "        char *src = slurp(file_path, &src_len);\n"
        "        if (!src) { fprintf(stderr, \"Failed to read input file: %s\\n\", file_path); cleanup_all_parsers(); return 5; }\n"
        "        rc = parse_source(file_path, src);\n"
        "        free(src);\n"
        "    } else if (expr_text) {\n"
        "        rc = parse_source(\"<cmdline>\", expr_text);\n"
        "    }\n"
        "\n"
        "    cleanup_all_parsers();\n"
        "    return rc;\n"
        "}\n",
        out);
}

/* ============================================================
 * CLI for the generator itself
 * ============================================================ */
static void usage_gen(const char *argv0)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s <path/to/grammar.mpc> > ../parser/parser_generated.c\n"
            "\n"
            "Reads the grammar file and emits a self-contained parser C file to stdout.\n",
            argv0);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        usage_gen(argv[0]);
        return 2;
    }

    const char *grammar_path = argv[1];

    size_t grammar_len = 0;
    char *grammar_src = slurp(grammar_path, &grammar_len);
    if (!grammar_src)
    {
        fprintf(stderr, "Failed to read grammar file: %s\n", grammar_path);
        return 3;
    }

    /* Collect rule names in order of appearance */
    StrVec rules;
    sv_init(&rules);
    collect_rules(grammar_src, grammar_len, &rules);

    /* Sanity: require at least 'program' to exist */
    if (!sv_contains(&rules, "program"))
    {
        fprintf(stderr, "Error: no 'program' rule found in grammar.\n");
        sv_free(&rules);
        free(grammar_src);
        return 4;
    }

    emit_parser_program(stdout, grammar_src, grammar_len, &rules);

    sv_free(&rules);
    free(grammar_src);
    return 0;
}
