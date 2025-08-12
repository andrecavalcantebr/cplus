/*
    FILE: main.c
    DESCR: Gera código C de um parser MPC a partir de um grammar.mpc (mpca_lang)
    AUTHOR: Andre Cavalcante
    DATE: August, 2025
    LICENSE: CC BY-SA
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* =========================================================================
 * util: ler arquivo inteiro
 * ========================================================================= */
static char *slurp(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    if (n < 0)
    {
        fclose(f);
        return NULL;
    }

    rewind(f);

    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf)
    {
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

/* =========================================================================
 * util: remover comentários //... e / * ... * /
 * (mantém quebras de linha)
 * ========================================================================= */
static void strip_comments(char *s)
{
    enum
    {
        NORM,
        SLASH,
        LINECMT,
        BLOCKCMT,
        STARINBLOCK
    } st = NORM;

    for (char *p = s; *p; ++p)
    {
        char c = *p;

        switch (st)
        {
        case NORM:
            if (c == '/')
            {
                st = SLASH;
            }
            break;

        case SLASH:
            if (c == '/')
            {
                *(p - 1) = ' ';
                *p = ' ';
                st = LINECMT;
            }
            else if (c == '*')
            {
                *(p - 1) = ' ';
                *p = ' ';
                st = BLOCKCMT;
            }
            else
            {
                st = NORM;
            }
            break;

        case LINECMT:
            if (*p != '\n')
            {
                *p = ' ';
            }
            else
            {
                st = NORM;
            }
            break;

        case BLOCKCMT:
            if (c != '\n')
            {
                *p = ' ';
            }
            if (c == '*')
            {
                st = STARINBLOCK;
            }
            break;

        case STARINBLOCK:
            if (*p != '\n')
            {
                *p = ' ';
            }
            if (c == '/')
            {
                st = NORM;
            }
            else if (c != '*')
            {
                st = BLOCKCMT;
            }
            break;
        }
    }
}

/* =========================================================================
 * util: normalizar nome para identificador C seguro
 * ========================================================================= */
static char *c_ident_from_rule(const char *name)
{
    size_t n = strlen(name);
    char *out = (char *)malloc(n + 2);
    if (!out)
        return NULL;

    size_t j = 0;
    for (size_t i = 0; i < n; ++i)
    {
        unsigned char c = (unsigned char)name[i];
        out[j++] = (isalnum(c) || c == '_') ? (char)c : '_';
    }
    out[j] = '\0';

    if (j > 0 && isdigit((unsigned char)out[0]))
    {
        memmove(out + 1, out, j + 1);
        out[0] = '_';
    }
    return out;
}

/* =========================================================================
 * coleta de não-terminais (LHS "nome : ... ;") na ordem de 1ª ocorrência
 * ========================================================================= */
typedef struct
{
    char **names;
    size_t n, cap;
} name_list;

static int contains(name_list *L, const char *name)
{
    for (size_t i = 0; i < L->n; ++i)
    {
        if (strcmp(L->names[i], name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static void push(name_list *L, const char *name)
{
    if (contains(L, name))
    {
        return;
    }
    if (L->n == L->cap)
    {
        size_t nc = L->cap ? L->cap * 2 : 8;
        char **nn = (char **)realloc(L->names, nc * sizeof(char *));
        if (!nn)
            return;
        L->names = nn;
        L->cap = nc;
    }
    L->names[L->n++] = strdup(name);
}

/* regra por linha: [ws] IDENT [ws] ':' */
static void collect_nonterminals(const char *text, name_list *out)
{
    const char *p = text;

    while (*p)
    {
        while (*p == ' ' || *p == '\t' || *p == '\r')
        {
            p++;
        }
        if (*p == '\n')
        {
            p++;
            continue;
        }

        const char *q = p;

        if (isalpha((unsigned char)*q) || *q == '_')
        {
            q++;
            while (isalnum((unsigned char)*q) || *q == '_')
            {
                q++;
            }

            const char *r = q;
            while (*r == ' ' || *r == '\t')
            {
                r++;
            }

            if (*r == ':')
            {
                size_t len = (size_t)(q - p);
                char *name = (char *)malloc(len + 1);
                memcpy(name, p, len);
                name[len] = '\0';
                push(out, name);
                free(name);
            }
        }

        while (*p && *p != '\n')
        {
            p++;
        }
        if (*p == '\n')
        {
            p++;
        }
    }
}

static int find_index(name_list *L, const char *name)
{
    for (size_t i = 0; i < L->n; ++i)
    {
        if (strcmp(L->names[i], name) == 0)
        {
            return (int)i;
        }
    }
    return -1;
}

/* Emite:
 *   static const char *GRAMMAR =
 *     "linha...\n"
 *     "linha...\n"
 *     ...
 *   ;
 * Escapa " e \ dentro das linhas. Normaliza CRLF para LF.
 */
static void emit_grammar_literal(const char *grammar_text)
{
    printf("static const char *GRAMMAR =\n");

    const char *p = grammar_text;
    while (*p)
    {
        /* pegar [p, q) como uma linha (sem CR/LF) */
        const char *q = p;
        while (*q && *q != '\n' && *q != '\r')
            q++;

        /* imprime prefixo e abre aspas */
        printf("  \"");

        /* conteúdo da linha, com escapes */
        for (const char *s = p; s < q; ++s)
        {
            unsigned char c = (unsigned char)*s;
            if (c == '\"')
            {
                printf("\\\"");
            }
            else if (c == '\\')
            {
                printf("\\\\");
            }
            else if (c >= 0x20 || c == '\t')
            {
                putchar(c);
            }
            else
            {
                /* raros controles — escape octal */
                printf("\\%03o", c);
            }
        }

        /* fecha string com \n e quebra de linha REAL fora da string */
        printf("\\n\"\n");

        /* pula CRLF, CR ou LF */
        if (*q == '\r' && q[1] == '\n')
            q += 2;
        else if (*q == '\r' || *q == '\n')
            q += 1;

        p = q;
    }

    printf(";\n\n");
}

/* =========================================================================
 * geração do código do parser (emite em stdout) — com blocos de strings
 * ========================================================================= */
static void generate_c(const char *grammar_path, char *raw_text, name_list *L)
{
    (void)grammar_path;

    /* ----- pré-processar gramática: tirar comentários ----- */
    strip_comments(raw_text);

    /* ----- Bloco: Cabeçalho do arquivo gerado ----- */
    printf(
        "/*\n"
        "\tFILE: parser_generated.c\n"
        "\tDESCR: Parser MPC gerado a partir de grammar.mpc via mpca_lang (gramática embutida)\n"
        "\tAUTHOR: Andre Cavalcante\n"
        "\tDATE: August, 2025\n"
        "\tLICENSE: CC BY-SA\n"
        "*/\n\n");

    /* ----- Bloco: Includes + util slurp (apenas para arquivo a parsear) ----- */
    printf(
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "#include <mpc.h>\n"
        "\n"
        "static char* slurp(const char *path) {\n"
        "  FILE *f = fopen(path, \"rb\");\n"
        "  if (!f) {\n"
        "    return NULL;\n"
        "  }\n"
        "\n"
        "  fseek(f, 0, SEEK_END);\n"
        "  long n = ftell(f);\n"
        "  if (n < 0) {\n"
        "    fclose(f);\n"
        "    return NULL;\n"
        "  }\n"
        "\n"
        "  rewind(f);\n"
        "  char *buf = (char*)malloc((size_t)n + 1);\n"
        "  if (!buf) {\n"
        "    fclose(f);\n"
        "    return NULL;\n"
        "  }\n"
        "\n"
        "  size_t rd = fread(buf, 1, (size_t)n, f);\n"
        "  fclose(f);\n"
        "\n"
        "  buf[rd] = '\\0';\n"
        "  return buf;\n"
        "}\n"
        "\n");

    /* ----- Bloco: literal da GRAMMAR embutida ----- */
    emit_grammar_literal(raw_text);

    /* ----- Bloco: Declarações dos parsers ----- */
    for (size_t i = 0; i < L->n; ++i)
    {
        char *cname = c_ident_from_rule(L->names[i]);
        printf("static mpc_parser_t *%s;\n", cname);
        free(cname);
    }
    printf("\n");

    /* ----- Bloco: build_parsers() com mpc_new ----- */
    printf(
        "static void build_parsers(void) {\n");
    for (size_t i = 0; i < L->n; ++i)
    {
        char *cname = c_ident_from_rule(L->names[i]);
        printf("  %s = mpc_new(\"%s\");\n", cname, L->names[i]);
        free(cname);
    }
    printf(
        "}\n"
        "\n");

    /* ----- Bloco: cleanup_parsers() com mpc_cleanup(N, ...) ----- */
    printf(
        "static void cleanup_parsers(void) {\n"
        "  mpc_cleanup(%zu",
        L->n);
    for (size_t i = 0; i < L->n; ++i)
    {
        char *cname = c_ident_from_rule(L->names[i]);
        printf(", %s", cname);
        free(cname);
    }
    printf(
        ");\n"
        "}\n"
        "\n");

    /* ----- Bloco: escolha da regra inicial (program se existir) ----- */
    int ix_program = find_index(L, "program");
    const char *start_rule = (ix_program >= 0) ? "program" : L->names[0];
    char *start_cname = c_ident_from_rule(start_rule);

    /* ----- Bloco: função main do parser gerado ----- */
    printf(
        "int main(int argc, char **argv) {\n"
        "  if (argc < 2) {\n"
        "    fprintf(stderr, \"Uso: %%s <arquivo_para_parsear>\\n\", argv[0]);\n"
        "    return 1;\n"
        "  }\n"
        "\n"
        "  char *input_text = slurp(argv[1]);\n"
        "  if (!input_text) {\n"
        "    fprintf(stderr, \"Erro ao ler arquivo de entrada: %%s\\n\", argv[1]);\n"
        "    return 1;\n"
        "  }\n"
        "\n"
        "  build_parsers();\n"
        "\n"
        "  mpc_err_t *err = mpca_lang(\n"
        "    MPCA_LANG_DEFAULT,\n"
        "    GRAMMAR");
    for (size_t i = 0; i < L->n; ++i)
    {
        char *cname = c_ident_from_rule(L->names[i]);
        printf(",\n    %s", cname);
        free(cname);
    }
    printf(
        ",\n"
        "    NULL\n"
        "  );\n"
        "\n"
        "  if (err) {\n"
        "    mpc_err_print(err);\n"
        "    mpc_err_delete(err);\n"
        "    free(input_text);\n"
        "    cleanup_parsers();\n"
        "    return 2;\n"
        "  }\n"
        "\n"
        "  mpc_result_t r;\n");

    /* Aqui interpolamos o símbolo inicial agora (printf real com %s) */
    printf(
        "  if (mpc_parse(\"%s\", input_text, %s, &r)) {\n",
        start_cname, start_cname);
    printf(
        "    mpc_ast_print(r.output);\n"
        "    mpc_ast_delete(r.output);\n"
        "  } else {\n"
        "    mpc_err_print(r.error);\n"
        "    mpc_err_delete(r.error);\n"
        "    free(input_text);\n"
        "    cleanup_parsers();\n"
        "    return 3;\n"
        "  }\n"
        "\n"
        "  free(input_text);\n"
        "  cleanup_parsers();\n"
        "  return 0;\n"
        "}\n");

    free(start_cname);
}

/* =========================================================================
 * main do gerador
 * ========================================================================= */
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s <caminho/para/grammar.mpc> > ../parser/parser_generated.c\n", argv[0]);
        return 1;
    }

    size_t n = 0;
    char *text = slurp(argv[1], &n);
    if (!text)
    {
        fprintf(stderr, "Erro ao ler arquivo: %s\n", argv[1]);
        return 1;
    }

    /* Não strip aqui para coleta de LHS? Fazemos strip
       depois, dentro do generate_c, antes de embutir literal. */
    name_list L = (name_list){0};
    collect_nonterminals(text, &L);

    if (L.n == 0)
    {
        fprintf(stderr, "Nenhum não-terminal encontrado (esperado: linhas do tipo 'nome : ... ;')\n");
        free(text);
        return 2;
    }

    generate_c(argv[1], text, &L);

    for (size_t i = 0; i < L.n; ++i)
    {
        free(L.names[i]);
    }
    free(L.names);
    free(text);
    return 0;
}
