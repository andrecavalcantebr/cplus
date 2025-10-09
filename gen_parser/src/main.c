/*
    FILE: main.c
    DESCR: Code generator for a self-contained Cplus parser (MPC/mpca_lang)
           Uses line comments starting with '#' (at column 0) in grammar.mpc.
           Generates a parser that embeds:
             - GRAMMAR_RAW (with # comments)
             - GRAMMAR     (without lines starting with '#')
           CLI of the generated parser:
             -G : print grammar with comments (RAW)
             -f : parse file
             -x : parse string
    AUTHOR: Andre Cavalcante
    DATE: September, 2025
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
    {
        if (strcmp(sv->v[i], s) == 0)
            return 1;
    }
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
 * Filter: remove lines that start with '#'
 * - Definition: line starts at column 0 with '#'
 * - Lines beginning with spaces + '#' are NOT treated as comments
 *   (by design; keeps rule unambiguous). If you want to allow leading
 *   spaces, check the first non-space instead.
 * - We DO NOT preserve removed lines (no newline inserted for them).
 * ============================================================ */
/* ============================================================
 * Filter: remove lines that start with '#'
 * - Lines beginning with '#' at column 0 are treated as comments.
 * - In the CLEAN grammar, we replace them with blank lines
 *   (just a '\n') to preserve line numbering.
 * ============================================================ */
static char *filter_hash_comment_lines(const char *src, size_t len, size_t *out_len)
{
    char *out = (char *)malloc(len + 1);
    if (!out)
    {
        fprintf(stderr, "OOM in filter_hash_comment_lines\n");
        return NULL;
    }

    size_t i = 0, o = 0;
    while (i < len)
    {
        size_t line_start = i;
        while (i < len && src[i] != '\n')
            i++;
        size_t line_end = i; /* points to '\n' or len */
        int has_nl = (i < len && src[i] == '\n');

        if (line_end > line_start && src[line_start] == '#')
        {
            /* emit blank line to preserve numbering */
            if (has_nl)
            {
                out[o++] = '\n';
                i++; /* consume '\n' */
            }
            else
            {
                /* last line without newline: just insert one */
                out[o++] = '\n';
                i = line_end;
            }
            continue;
        }

        /* copy this line normally */
        size_t L = line_end - line_start;
        if (L)
        {
            memcpy(out + o, src + line_start, L);
            o += L;
        }
        if (has_nl)
        {
            out[o++] = '\n';
            i++;
        }
    }

    out[o] = '\0';
    if (out_len)
        *out_len = o;
    return out;
}

/* ============================================================
 * Collect nonterminals (rule names) in order from grammar text
 * Assumes format: <rule> : ... ;
 * Finds ':' outside quotes/regex and takes the identifier
 * immediately before ':'.
 * ============================================================ */
static int is_ident_start(int c) { return c == '_' || isalpha(c); }
static int is_ident(int c) { return c == '_' || isalnum(c); }

static void collect_rules(const char *g, size_t glen, StrVec *rules)
{
    enum
    {
        NORM,
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
            if (c == '\'')
            {
                st = SQ;
                i++;
                esc = 0;
                break;
            }
            if (c == '\"')
            {
                st = DQ;
                i++;
                esc = 0;
                break;
            }
            if (c == '/')
            {
                st = REGEX;
                i++;
                esc = 0;
                break;
            } /* MPC regex */
            if (c == ':')
            {
                size_t j = i;
                while (j > 0 && isspace((unsigned char)g[j - 1]))
                    j--;
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
                break;
            }
            i++;
            break;

        case SQ:
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

        case DQ:
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

        case REGEX:
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
 * Emit a C string literal with a given symbol name
 * ============================================================ */
static void emit_c_string_literal_named(FILE *out, const char *symname,
                                        const char *src, size_t len)
{
    fprintf(out, "static const char *%s =\n", symname);
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
        fputs("\"\n", out);
    fputs(";\n\n", out);
}

/* ============================================================
 * Emit the generated parser program to stdout.
 * - Embeds GRAMMAR_RAW (with # comments) and GRAMMAR (clean)
 * - mpca_lang uses GRAMMAR
 * - -G prints GRAMMAR_RAW
 * ============================================================ */
static void emit_parser_program(FILE *out,
                                const char *grammar_src_raw, size_t grammar_len_raw,
                                const char *grammar_src_clean, size_t grammar_len_clean,
                                const StrVec *rules)
{
    /* Header and includes */
    fputs(
        "/*\n"
        "    FILE: parser_generated.c\n"
        "    DESCR: Self-contained Cplus parser (MPC/mpca_lang) with CLI\n"
        "            -G : print grammar (with # comments)\n"
        "            -f : parse file\n"
        "            -x : parse string\n"
        "    AUTHOR: Generated by gen_parser\n"
        "    DATE: September, 2025\n"
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

    /* Embedded grammar strings */
    emit_c_string_literal_named(out, "GRAMMAR_RAW", grammar_src_raw, grammar_len_raw);
    emit_c_string_literal_named(out, "GRAMMAR", grammar_src_clean, grammar_len_clean);

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
        "        \"  -G          Print the embedded grammar (with # comments) and exit\\n\"\n"
        "        \"  -f <path>   Parse the given Cplus header/source file\\n\"\n"
        "        \"  -x <text>   Parse the given text directly\\n\"\n"
        "        \"  -d <outdir> Directory to write generated C files (default=.)\\n\",\n"
        "        argv0, argv0, argv0);\n"
        "}\n\n",
        out);

    /* build_all_parsers(): mpc_new for each, mpca_lang with ...rules..., NULL */
    fputs("static int build_all_parsers(void) {\n", out);
    for (size_t i = 0; i < rules->n; ++i)
    {
        fprintf(out, "    %s = mpc_new(\"%s\");\n", rules->v[i], rules->v[i]);
    }
    fputs("    /* mpca_lang returns mpc_err_t* (NULL on success) */\n", out);
    /* Use whitespace-sensitive mode; grammar controls spaces via <skips> */
    fputs("    mpc_err_t *err = mpca_lang(MPCA_LANG_WHITESPACE_SENSITIVE, GRAMMAR,\n", out);
    for (size_t i = 0; i < rules->n; ++i)
    {
        fprintf(out, "        %s,\n", rules->v[i]);
    }
    fputs("        NULL);\n", out);
    fputs(
        "    if (err) {\n"
        "        mpc_err_print(err);\n"
        "        mpc_err_delete(err);\n"
        "        return 0;\n"
        "    }\n"
        "    return 1;\n"
        "}\n\n",
        out);

    /* cleanup_all_parsers(): mpc_cleanup(N, ...) */
    fputs("static void cleanup_all_parsers(void) {\n", out);
    fprintf(out, "    mpc_cleanup(%zu", rules->n);
    for (size_t i = 0; i < rules->n; ++i)
    {
        fprintf(out, ", %s", rules->v[i]);
    }
    fputs(");\n}\n\n", out);

    /* parse_source(): uses 'program' rule explicitly */
    fputs(
        "static int parse_source(const char *input_name, const char *source, const char *output_dir) {\n"
        "    mpc_result_t r;\n"
        "    if (mpc_parse(input_name, source, program, &r)) {\n"
        "        puts(\"== PARSE SUCCESS ==\");\n"
        "        ast_transformation(r.output, output_dir);\n"
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

    /* main() of the generated parser */
    fputs(
        "int main(int argc, char **argv) {\n"
        "    const char *file_path = NULL;\n"
        "    const char *expr_text = NULL;\n"
        "    const char *output_dir = \".\";\n"
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
        "        } else if (strcmp(argv[i], \"-d\") == 0) {\n"
        "            if (i + 1 >= argc) { usage(argv[0]); return 2; }\n"
        "            output_dir = argv[++i];\n"
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
        "    if (print_grammar) { fputs(GRAMMAR_RAW, stdout); return 0; }\n"
        "\n"
        "    if (!build_all_parsers()) {\n"
        "        cleanup_all_parsers();\n"
        "        return 4;\n"
        "    }\n"
        "\n"
        "    int rc = 0;\n"
        "    if (file_path) {\n"
        "        size_t src_len = 0;\n"
        "        char *src = slurp(file_path, &src_len);\n"
        "        if (!src) {\n"
        "            fprintf(stderr, \"Failed to read input file: %s\\n\", file_path);\n"
        "            cleanup_all_parsers();\n"
        "            return 5;\n"
        "        }\n"
        "        rc = parse_source(file_path, src, output_dir);\n"
        "        free(src);\n"
        "    } else if (expr_text) {\n"
        "        rc = parse_source(\"<cmdline>\", expr_text, output_dir);\n"
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

    /* Load RAW grammar (with '#' comment lines) */
    size_t grammar_len_raw = 0;
    char *grammar_src_raw = slurp(grammar_path, &grammar_len_raw);
    if (!grammar_src_raw)
    {
        fprintf(stderr, "Failed to read grammar file: %s\n", grammar_path);
        return 3;
    }

    /* Build CLEAN grammar: remove lines starting with '#' */
    size_t grammar_len_clean = 0;
    char *grammar_src_clean = filter_hash_comment_lines(grammar_src_raw, grammar_len_raw, &grammar_len_clean);
    if (!grammar_src_clean)
    {
        fprintf(stderr, "Failed to filter # comment lines.\n");
        free(grammar_src_raw);
        return 5;
    }

    //    Uncomment to debug grammars
    //    printf("GRAMMAR_RAW:\n%s\n\nGRAMMAR CLEAN:\n%s\n\n", grammar_src_raw, grammar_src_clean);
    //    return 0;

    /* Collect rule names from CLEAN grammar (so commented rules don't count) */
    StrVec rules;
    sv_init(&rules);
    collect_rules(grammar_src_clean, grammar_len_clean, &rules);

    /* Require 'program' rule */
    if (!sv_contains(&rules, "program"))
    {
        fprintf(stderr, "Error: no 'program' rule found in grammar (after removing # comment lines).\n");
        sv_free(&rules);
        free(grammar_src_clean);
        free(grammar_src_raw);
        return 4;
    }

    /* Emit generated parser source to stdout */
    emit_parser_program(stdout,
                        grammar_src_raw, grammar_len_raw,
                        grammar_src_clean, grammar_len_clean,
                        &rules);

    sv_free(&rules);
    free(grammar_src_clean);
    free(grammar_src_raw);
    return 0;
}
