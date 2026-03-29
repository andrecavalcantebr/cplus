/*
 * FILE: lexer.c
 * DESC.: island scanner implementation for cplus v2.
 *        Classifies cplus keywords and structural tokens; everything else is
 *        returned verbatim as TK_PASSTHROUGH blobs.
 *        Strings, character literals, line comments, block comments, and
 *        preprocessor directives are consumed as opaque passthrough so that
 *        cplus keywords embedded in them are never misidentified.
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#include "lexer.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* ---------------------------------------------------------------------- */
/* Internal helpers                                                         */
/* ---------------------------------------------------------------------- */

static bool is_alpha_or_underscore(char c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'));
}

static bool is_alnum_or_underscore(char c) {
    return (is_alpha_or_underscore(c) || (c >= '0' && c <= '9'));
}

static bool is_digit(char c) {
    return (c >= '0' && c <= '9');
}

/* Peek at the current character without advancing. Returns '\0' at EOF. */
static char cur(const Lexer *lex) {
    if (lex->pos >= lex->src_len) {
        return '\0';
    }
    return lex->src[lex->pos];
}

/* Peek one character ahead. Returns '\0' if out of bounds. */
static char peek1(const Lexer *lex) {
    if ((lex->pos + 1U) >= lex->src_len) {
        return '\0';
    }
    return lex->src[lex->pos + 1U];
}

/* Advance the cursor by one, updating line/col tracking. */
static void advance(Lexer *lex) {
    if (lex->pos >= lex->src_len) {
        return;
    }
    char c = lex->src[lex->pos];
    lex->pos++;
    if (c == '\n') {
        lex->line++;
        lex->col = 1;
        lex->at_bol = true;
    } else {
        lex->col++;
        lex->at_bol = false;
    }
}

/* Classify an identifier spelling against the cplus keyword table. */
static TokenKind classify_ident(const char *start, size_t len) {
    if (len == 4U && memcmp(start, "weak",   4U) == 0) { return TK_WEAK;   }
    if (len == 6U && memcmp(start, "unique", 6U) == 0) { return TK_UNIQUE; }
    if (len == 4U && memcmp(start, "move",   4U) == 0) { return TK_MOVE;   }
    if (len == 5U && memcmp(start, "class",  5U) == 0) { return TK_CLASS;  }
    return TK_IDENT;
}

/* ---------------------------------------------------------------------- */
/* Passthrough accumulators — consume source ranges that must not be        */
/* interpreted as cplus tokens (strings, comments, CPP directives).         */
/* Each function advances lex->pos past the entire construct.               */
/* ---------------------------------------------------------------------- */

/* Consume a double-quoted string literal, including the closing '"'.
 * Caller verified cur() == '"' before calling. */
static void consume_dq_string(Lexer *lex) {
    advance(lex); /* skip opening '"' */
    while (lex->pos < lex->src_len) {
        char c = cur(lex);
        if (c == '\\') {
            advance(lex); /* skip backslash */
            if (lex->pos < lex->src_len) {
                advance(lex); /* skip escaped char */
            }
        } else if (c == '"') {
            advance(lex); /* skip closing '"' */
            break;
        } else {
            advance(lex);
        }
    }
}

/* Consume a single-quoted character literal, including the closing '\''. */
static void consume_sq_string(Lexer *lex) {
    advance(lex); /* skip opening '\'' */
    while (lex->pos < lex->src_len) {
        char c = cur(lex);
        if (c == '\\') {
            advance(lex);
            if (lex->pos < lex->src_len) {
                advance(lex);
            }
        } else if (c == '\'') {
            advance(lex);
            break;
        } else {
            advance(lex);
        }
    }
}

/* Consume a // line comment (up to and including the newline). */
static void consume_line_comment(Lexer *lex) {
    while (lex->pos < lex->src_len && cur(lex) != '\n') {
        advance(lex);
    }
    if (lex->pos < lex->src_len) {
        advance(lex); /* consume the \n */
    }
}

/* Consume a block comment.  Caller verified cur()=='/'; peek1()=='*'. */
static void consume_block_comment(Lexer *lex) {
    advance(lex); /* '/' */
    advance(lex); /* '*' */
    while (lex->pos < lex->src_len) {
        if (cur(lex) == '*' && peek1(lex) == '/') {
            advance(lex); /* '*' */
            advance(lex); /* '/' */
            break;
        }
        advance(lex);
    }
}

/* Consume a preprocessor directive (#...) to end of logical line.
 * Handles line-continuation (\<newline>).
 * Caller verified at_bol and cur()=='#'. */
static void consume_cpp_directive(Lexer *lex) {
    while (lex->pos < lex->src_len) {
        char c = cur(lex);
        if (c == '\\' && peek1(lex) == '\n') {
            advance(lex); /* '\' */
            advance(lex); /* '\n' — line continuation, keep consuming */
        } else if (c == '\n') {
            advance(lex);
            break;
        } else {
            advance(lex);
        }
    }
}

/* ---------------------------------------------------------------------- */
/* Core scanner                                                             */
/* ---------------------------------------------------------------------- */

void lexer_init(Lexer *lex, const char *src, size_t src_len) {
    lex->src        = src;
    lex->src_len    = src_len;
    lex->pos        = 0U;
    lex->line       = 1;
    lex->col        = 1;
    lex->at_bol     = true;
    lex->has_peeked = false;
    /* peeked is left uninitialised — only valid when has_peeked == true */
}

/*
 * lexer_next_raw() — the actual scanning logic, called only by lexer_next()
 * after the peek-slot check.
 *
 * Strategy: accumulate passthrough text in [pt_start, pt_start+pt_len).
 * When we encounter a boundary (cplus keyword, structural token, or EOF) we
 * flush any pending passthrough first, then handle the boundary event.
 */
static Token lexer_next_raw(Lexer *lex) {
    /* Passthrough accumulation state */
    const char *pt_start = NULL;
    size_t      pt_len   = 0U;
    int         pt_line  = 0;
    int         pt_col   = 0;

#define FLUSH_PASSTHROUGH(out)                        \
    do {                                              \
        (out).kind  = TK_PASSTHROUGH;                 \
        (out).start = pt_start;                       \
        (out).len   = pt_len;                         \
        (out).line  = pt_line;                        \
        (out).col   = pt_col;                         \
        pt_start    = NULL;                           \
        pt_len      = 0U;                             \
    } while (0)

#define ACCUM(n)                                      \
    do {                                              \
        if (pt_start == NULL) {                       \
            pt_start = lex->src + lex->pos;           \
            pt_line  = lex->line;                     \
            pt_col   = lex->col;                      \
        }                                             \
        size_t _before = lex->pos;                    \
        (void)_before;                                \
        (n);                                          \
        pt_len += lex->pos - _before;                 \
    } while (0)

    while (lex->pos < lex->src_len) {
        char c  = cur(lex);
        char c1 = peek1(lex);

        /* ---- line comment ------------------------------------------- */
        if (c == '/' && c1 == '*') {
            size_t before = lex->pos;
            if (pt_start == NULL) { pt_start = lex->src + lex->pos; pt_line = lex->line; pt_col = lex->col; }
            consume_block_comment(lex);
            pt_len += lex->pos - before;
            continue;
        }

        if (c == '/' && c1 == '/') {
            size_t before = lex->pos;
            if (pt_start == NULL) { pt_start = lex->src + lex->pos; pt_line = lex->line; pt_col = lex->col; }
            consume_line_comment(lex);
            pt_len += lex->pos - before;
            continue;
        }

        /* ---- preprocessor directive ---------------------------------- */
        if (c == '#' && lex->at_bol) {
            size_t before = lex->pos;
            if (pt_start == NULL) { pt_start = lex->src + lex->pos; pt_line = lex->line; pt_col = lex->col; }
            consume_cpp_directive(lex);
            pt_len += lex->pos - before;
            continue;
        }

        /* ---- double-quoted string ------------------------------------ */
        if (c == '"') {
            size_t before = lex->pos;
            if (pt_start == NULL) { pt_start = lex->src + lex->pos; pt_line = lex->line; pt_col = lex->col; }
            consume_dq_string(lex);
            pt_len += lex->pos - before;
            continue;
        }

        /* ---- single-quoted char literal ------------------------------ */
        if (c == '\'') {
            size_t before = lex->pos;
            if (pt_start == NULL) { pt_start = lex->src + lex->pos; pt_line = lex->line; pt_col = lex->col; }
            consume_sq_string(lex);
            pt_len += lex->pos - before;
            continue;
        }

        /* ---- digit — accumulate as passthrough ----------------------- */
        if (is_digit(c)) {
            size_t before = lex->pos;
            if (pt_start == NULL) { pt_start = lex->src + lex->pos; pt_line = lex->line; pt_col = lex->col; }
            while (lex->pos < lex->src_len && is_alnum_or_underscore(cur(lex))) {
                advance(lex);
            }
            pt_len += lex->pos - before;
            continue;
        }

        /* ---- identifier or keyword ----------------------------------- */
        if (is_alpha_or_underscore(c)) {
            /* Flush any pending passthrough before emitting this token */
            if (pt_len > 0U) {
                Token out;
                FLUSH_PASSTHROUGH(out);
                /* We haven't advanced yet — the peek-slot will capture the ident
                 * on the next call.  We store the passthrough in the peek slot
                 * and return it now. */
                lex->peeked.kind  = TK_EOF; /* will be replaced */
                lex->has_peeked   = false;  /* not used here */
                return out;
            }

            /* Read the complete identifier */
            const char *id_start = lex->src + lex->pos;
            int         id_line  = lex->line;
            int         id_col   = lex->col;
            while (lex->pos < lex->src_len && is_alnum_or_underscore(cur(lex))) {
                advance(lex);
            }
            size_t id_len = (size_t)(lex->pos - (size_t)(id_start - lex->src));

            Token out;
            out.kind  = classify_ident(id_start, id_len);
            out.start = id_start;
            out.len   = id_len;
            out.line  = id_line;
            out.col   = id_col;
            return out;
        }

        /* ---- structural single-char token ---------------------------- */
        {
            TokenKind sk = TK_EOF; /* sentinel: not a structural token */
            switch (c) {
                case '*': sk = TK_STAR;      break;
                case '&': sk = TK_AMPERSAND; break;
                case '{': sk = TK_LBRACE;    break;
                case '}': sk = TK_RBRACE;    break;
                case '(': sk = TK_LPAREN;    break;
                case ')': sk = TK_RPAREN;    break;
                case ';': sk = TK_SEMI;      break;
                case '=': sk = TK_ASSIGN;    break;
                default:  sk = TK_EOF;       break;
            }

            if (sk != TK_EOF) {
                /* Flush passthrough first */
                if (pt_len > 0U) {
                    /* Save the structural-token position, flush passthrough,
                     * store the structural token in the peek slot, return PT. */
                    int sc_line = lex->line;
                    int sc_col  = lex->col;
                    advance(lex);

                    /* Store structural token in peek slot */
                    lex->peeked.kind  = sk;
                    lex->peeked.start = lex->src + lex->pos - 1U;
                    lex->peeked.len   = 1U;
                    lex->peeked.line  = sc_line;
                    lex->peeked.col   = sc_col;
                    lex->has_peeked   = true;

                    Token out;
                    FLUSH_PASSTHROUGH(out);
                    return out;
                }

                /* No pending passthrough — emit the structural token directly */
                Token out;
                out.kind  = sk;
                out.start = lex->src + lex->pos;
                out.len   = 1U;
                out.line  = lex->line;
                out.col   = lex->col;
                advance(lex);
                return out;
            }
        }

        /* ---- everything else: accumulate as passthrough -------------- */
        {
            size_t before = lex->pos;
            if (pt_start == NULL) { pt_start = lex->src + lex->pos; pt_line = lex->line; pt_col = lex->col; }
            advance(lex);
            pt_len += lex->pos - before;
        }
    } /* end while */

    /* EOF reached */
    if (pt_len > 0U) {
        Token out;
        FLUSH_PASSTHROUGH(out);
        return out;
    }

    Token eof;
    eof.kind  = TK_EOF;
    eof.start = lex->src + lex->src_len;
    eof.len   = 0U;
    eof.line  = lex->line;
    eof.col   = lex->col;
    return eof;

#undef FLUSH_PASSTHROUGH
#undef ACCUM
}

Token lexer_next(Lexer *lex) {
    if (lex->has_peeked) {
        lex->has_peeked = false;
        return lex->peeked;
    }
    return lexer_next_raw(lex);
}

Token lexer_peek(Lexer *lex) {
    if (!lex->has_peeked) {
        lex->peeked     = lexer_next_raw(lex);
        lex->has_peeked = true;
    }
    return lex->peeked;
}

const char *lexer_token_kind_name(TokenKind kind) {
    switch (kind) {
        case TK_EOF:         return "TK_EOF";
        case TK_PASSTHROUGH: return "TK_PASSTHROUGH";
        case TK_IDENT:       return "TK_IDENT";
        case TK_WEAK:        return "TK_WEAK";
        case TK_UNIQUE:      return "TK_UNIQUE";
        case TK_MOVE:        return "TK_MOVE";
        case TK_CLASS:       return "TK_CLASS";
        case TK_STAR:        return "TK_STAR";
        case TK_AMPERSAND:   return "TK_AMPERSAND";
        case TK_LBRACE:      return "TK_LBRACE";
        case TK_RBRACE:      return "TK_RBRACE";
        case TK_LPAREN:      return "TK_LPAREN";
        case TK_RPAREN:      return "TK_RPAREN";
        case TK_SEMI:        return "TK_SEMI";
        case TK_ASSIGN:      return "TK_ASSIGN";
    }
    return "TK_UNKNOWN";
}
