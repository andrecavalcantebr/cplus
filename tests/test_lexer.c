/*
 * FILE: test_lexer.c
 * DESC.: unit tests for the cplus island lexer (src/lexer.{h,c}).
 *        All test inputs are inline strings — no fixtures are needed.
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#include "lexer.h"

#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------------- */
/* Minimal test harness                                                     */
/* ---------------------------------------------------------------------- */

static int g_failures = 0;

#define ASSERT(cond, msg)                                                \
    do {                                                                 \
        if (!(cond)) {                                                   \
            fprintf(stderr, "FAIL [%s] line %d: %s\n",                  \
                    __func__, __LINE__, (msg));                          \
            g_failures++;                                                \
        }                                                                \
    } while (0)

/* Check kind and, when expected_text != NULL, the literal token text. */
static void check_tok(const char *test_name, Token tok, TokenKind expected_kind,
                       const char *expected_text) {
    if (tok.kind != expected_kind) {
        fprintf(stderr, "FAIL [%s] expected %s got %s\n",
                test_name,
                lexer_token_kind_name(expected_kind),
                lexer_token_kind_name(tok.kind));
        g_failures++;
        return;
    }
    if (expected_text != NULL) {
        size_t exp_len = strlen(expected_text);
        if (tok.len != exp_len || memcmp(tok.start, expected_text, exp_len) != 0) {
            fprintf(stderr, "FAIL [%s] token text: expected \"%s\" got \"%.*s\"\n",
                    test_name, expected_text, (int)tok.len, tok.start);
            g_failures++;
        }
    }
}

/* ---------------------------------------------------------------------- */
/* Test cases                                                               */
/* ---------------------------------------------------------------------- */

/* 1. Empty source → TK_EOF immediately */
static void test_empty_source(void) {
    Lexer lex;
    lexer_init(&lex, "", 0U);
    Token tok = lexer_next(&lex);
    check_tok(__func__, tok, TK_EOF, NULL);
}

/* 2. Keyword weak */
static void test_keyword_weak(void) {
    const char *src = "weak";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));
    check_tok(__func__, lexer_next(&lex), TK_WEAK, "weak");
    check_tok(__func__, lexer_next(&lex), TK_EOF,  NULL);
}

/* 3. Keyword unique */
static void test_keyword_unique(void) {
    const char *src = "unique";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));
    check_tok(__func__, lexer_next(&lex), TK_UNIQUE, "unique");
    check_tok(__func__, lexer_next(&lex), TK_EOF,    NULL);
}

/* 4. Keyword move */
static void test_keyword_move(void) {
    const char *src = "move";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));
    check_tok(__func__, lexer_next(&lex), TK_MOVE, "move");
    check_tok(__func__, lexer_next(&lex), TK_EOF,  NULL);
}

/* 5. Keyword struct */
static void test_keyword_struct(void) {
    const char *src = "struct";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));
    check_tok(__func__, lexer_next(&lex), TK_STRUCT, "struct");
    check_tok(__func__, lexer_next(&lex), TK_EOF,    NULL);
}

/* 6. Non-keyword identifier */
static void test_ident(void) {
    const char *src = "foobar";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));
    check_tok(__func__, lexer_next(&lex), TK_IDENT, "foobar");
    check_tok(__func__, lexer_next(&lex), TK_EOF,   NULL);
}

/* 7. Structural single-character tokens */
static void test_structural_tokens(void) {
    const char *src = "*&{}();=";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));
    check_tok(__func__, lexer_next(&lex), TK_STAR,      "*");
    check_tok(__func__, lexer_next(&lex), TK_AMPERSAND, "&");
    check_tok(__func__, lexer_next(&lex), TK_LBRACE,    "{");
    check_tok(__func__, lexer_next(&lex), TK_RBRACE,    "}");
    check_tok(__func__, lexer_next(&lex), TK_LPAREN,    "(");
    check_tok(__func__, lexer_next(&lex), TK_RPAREN,    ")");
    check_tok(__func__, lexer_next(&lex), TK_SEMI,      ";");
    check_tok(__func__, lexer_next(&lex), TK_ASSIGN,    "=");
    check_tok(__func__, lexer_next(&lex), TK_EOF,       NULL);
}

/* 8. Keyword inside double-quoted string → TK_PASSTHROUGH (not TK_WEAK) */
static void test_keyword_in_dq_string(void) {
    const char *src = "\"weak\"";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));
    Token tok = lexer_next(&lex);
    check_tok(__func__, tok, TK_PASSTHROUGH, "\"weak\"");
    check_tok(__func__, lexer_next(&lex), TK_EOF, NULL);
}

/* 9. Keyword inside // line comment → TK_PASSTHROUGH */
static void test_keyword_in_line_comment(void) {
    const char *src = "// weak\n";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));
    Token tok = lexer_next(&lex);
    check_tok(__func__, tok, TK_PASSTHROUGH, "// weak\n");
    check_tok(__func__, lexer_next(&lex), TK_EOF, NULL);
}

/* 10. Keyword inside block comment → TK_PASSTHROUGH */
static void test_keyword_in_block_comment(void) {
    const char *src = "/* weak */";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));
    Token tok = lexer_next(&lex);
    check_tok(__func__, tok, TK_PASSTHROUGH, "/* weak */");
    check_tok(__func__, lexer_next(&lex), TK_EOF, NULL);
}

/* 11. Preprocessor directive → TK_PASSTHROUGH
 *     The directive must be at the start of the string (at_bol == true). */
static void test_cpp_directive(void) {
    const char *src = "#include <stdio.h>\n";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));
    Token tok = lexer_next(&lex);
    check_tok(__func__, tok, TK_PASSTHROUGH, "#include <stdio.h>\n");
    check_tok(__func__, lexer_next(&lex), TK_EOF, NULL);
}

/* 12. Full struct declaration sequence
 *     Input: "struct Person { int age; };"
 *     Expected token stream:
 *       TK_STRUCT("struct")
 *       TK_PASSTHROUGH(" ")
 *       TK_IDENT("Person")
 *       TK_PASSTHROUGH(" ")
 *       TK_LBRACE("{")
 *       TK_PASSTHROUGH(" ")
 *       TK_IDENT("int")
 *       TK_PASSTHROUGH(" ")
 *       TK_IDENT("age")
 *       TK_SEMI(";")
 *       TK_PASSTHROUGH(" ")
 *       TK_RBRACE("}")
 *       TK_SEMI(";")
 *       TK_EOF
 */
static void test_struct_sequence(void) {
    const char *src = "struct Person { int age; };";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));

    check_tok(__func__, lexer_next(&lex), TK_STRUCT,      "struct");
    check_tok(__func__, lexer_next(&lex), TK_PASSTHROUGH, " ");
    check_tok(__func__, lexer_next(&lex), TK_IDENT,       "Person");
    check_tok(__func__, lexer_next(&lex), TK_PASSTHROUGH, " ");
    check_tok(__func__, lexer_next(&lex), TK_LBRACE,      "{");
    check_tok(__func__, lexer_next(&lex), TK_PASSTHROUGH, " ");
    check_tok(__func__, lexer_next(&lex), TK_IDENT,       "int");
    check_tok(__func__, lexer_next(&lex), TK_PASSTHROUGH, " ");
    check_tok(__func__, lexer_next(&lex), TK_IDENT,       "age");
    check_tok(__func__, lexer_next(&lex), TK_SEMI,        ";");
    check_tok(__func__, lexer_next(&lex), TK_PASSTHROUGH, " ");
    check_tok(__func__, lexer_next(&lex), TK_RBRACE,      "}");
    check_tok(__func__, lexer_next(&lex), TK_SEMI,        ";");
    check_tok(__func__, lexer_next(&lex), TK_EOF,         NULL);
}

/* 13. lexer_peek() does not advance the stream
 *     Two consecutive peeks return the same token; next() consumes it. */
static void test_peek_non_advancing(void) {
    const char *src = "weak";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));

    Token p1 = lexer_peek(&lex);
    Token p2 = lexer_peek(&lex);

    check_tok(__func__, p1, TK_WEAK, "weak");
    check_tok(__func__, p2, TK_WEAK, "weak");

    /* Verify same pointer — both peeks returned the exact same token text */
    ASSERT(p1.start == p2.start, "peek inconsistency: start differs");

    /* Consuming advances past the token */
    check_tok(__func__, lexer_next(&lex), TK_WEAK, "weak");
    check_tok(__func__, lexer_next(&lex), TK_EOF,  NULL);
}

/* 14. Digit sequence and star: "42 *p;"
 *     Digits are opaque passthrough; the star is a structural token.
 *     Expected: TK_PASSTHROUGH("42 "), TK_STAR, TK_IDENT("p"), TK_SEMI, TK_EOF */
static void test_digit_passthrough_and_star(void) {
    const char *src = "42 *p;";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));

    check_tok(__func__, lexer_next(&lex), TK_PASSTHROUGH, "42 ");
    check_tok(__func__, lexer_next(&lex), TK_STAR,        "*");
    check_tok(__func__, lexer_next(&lex), TK_IDENT,       "p");
    check_tok(__func__, lexer_next(&lex), TK_SEMI,        ";");
    check_tok(__func__, lexer_next(&lex), TK_EOF,         NULL);
}

/* 15. "==" produces two TK_ASSIGN tokens (no lookahead merging) */
static void test_double_assign(void) {
    const char *src = "==";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));

    check_tok(__func__, lexer_next(&lex), TK_ASSIGN, "=");
    check_tok(__func__, lexer_next(&lex), TK_ASSIGN, "=");
    check_tok(__func__, lexer_next(&lex), TK_EOF,    NULL);
}

/* 16. Whitespace between cplus keywords is preserved in TK_PASSTHROUGH */
static void test_whitespace_passthrough(void) {
    const char *src = "weak    unique";
    Lexer lex;
    lexer_init(&lex, src, strlen(src));

    check_tok(__func__, lexer_next(&lex), TK_WEAK,        "weak");
    check_tok(__func__, lexer_next(&lex), TK_PASSTHROUGH, "    ");
    check_tok(__func__, lexer_next(&lex), TK_UNIQUE,      "unique");
    check_tok(__func__, lexer_next(&lex), TK_EOF,         NULL);
}

/* ---------------------------------------------------------------------- */
/* Entry point                                                              */
/* ---------------------------------------------------------------------- */

int main(void) {
    test_empty_source();
    test_keyword_weak();
    test_keyword_unique();
    test_keyword_move();
    test_keyword_struct();
    test_ident();
    test_structural_tokens();
    test_keyword_in_dq_string();
    test_keyword_in_line_comment();
    test_keyword_in_block_comment();
    test_cpp_directive();
    test_struct_sequence();
    test_peek_non_advancing();
    test_digit_passthrough_and_star();
    test_double_assign();
    test_whitespace_passthrough();

    if (g_failures > 0) {
        fprintf(stderr, "%d test(s) failed\n", g_failures);
        return 1;
    }

    return 0;
}
