/*
 * FILE: lexer.h
 * DESC.: island scanner for cplus v2 — tokenises .cplus/.hplus source into
 *        a pull-based stream of Token values.  Only cplus-specific constructs
 *        are classified; all other text is returned verbatim as TK_PASSTHROUGH.
 *        Tokens never allocate: Token.start points into the original source buffer.
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#ifndef CPLUS_LEXER_H
#define CPLUS_LEXER_H

#include <stdbool.h>
#include <stddef.h>

/* ---------------------------------------------------------------------- */
/* Token kinds                                                              */
/* ---------------------------------------------------------------------- */

typedef enum {
    TK_EOF,          /* end of source                                      */
    TK_PASSTHROUGH,  /* verbatim C23 text not recognized as a cplus token  */
    TK_IDENT,        /* non-keyword identifier                             */

    /* cplus keywords */
    TK_WEAK,         /* weak                                               */
    TK_UNIQUE,       /* unique                                             */
    TK_MOVE,         /* move                                               */
    TK_STRUCT,       /* struct (at file scope → auto-typedef)              */

    /* single-character structural tokens */
    TK_STAR,         /* *                                                  */
    TK_AMPERSAND,    /* &                                                  */
    TK_LBRACE,       /* {                                                  */
    TK_RBRACE,       /* }                                                  */
    TK_LPAREN,       /* (                                                  */
    TK_RPAREN,       /* )                                                  */
    TK_SEMI,         /* ;                                                  */
    TK_ASSIGN        /* =                                                  */
} TokenKind;

/* ---------------------------------------------------------------------- */
/* Token                                                                    */
/* ---------------------------------------------------------------------- */

/*
 * A single token.  `start` points into the source buffer passed to
 * lexer_init(); the Lexer (and caller) must keep that buffer alive for the
 * lifetime of any Token that references it.  No heap allocation is performed.
 */
typedef struct {
    TokenKind    kind;
    const char  *start;  /* pointer into source buffer; NOT NUL-terminated */
    size_t       len;    /* byte length of the token text                  */
    int          line;   /* 1-based source line of the first byte          */
    int          col;    /* 1-based source column of the first byte        */
} Token;

/* ---------------------------------------------------------------------- */
/* Lexer                                                                    */
/* ---------------------------------------------------------------------- */

typedef struct {
    const char *src;       /* source buffer (not owned)                   */
    size_t      src_len;   /* length in bytes                             */
    size_t      pos;       /* current read position                       */
    int         line;      /* current 1-based line                        */
    int         col;       /* current 1-based column                      */
    bool        at_bol;    /* true when cursor is at beginning-of-line    */

    /* single-token lookahead slot for lexer_peek() */
    Token       peeked;
    bool        has_peeked;
} Lexer;

/* ---------------------------------------------------------------------- */
/* Public API                                                               */
/* ---------------------------------------------------------------------- */

/*
 * Initialise a Lexer over `src` (a buffer of `src_len` bytes).
 * The buffer is not copied; it must remain valid for the lifetime of the Lexer.
 * `src` does not need to be NUL-terminated.
 */
void lexer_init(Lexer *lex, const char *src, size_t src_len);

/*
 * Consume and return the next token from the stream.
 * Returns a token with kind == TK_EOF when the source is exhausted.
 * Never returns NULL.
 */
Token lexer_next(Lexer *lex);

/*
 * Return the next token WITHOUT consuming it.
 * Repeated calls to lexer_peek() without an intervening lexer_next() return
 * the same token.
 */
Token lexer_peek(Lexer *lex);

/*
 * Return a stable human-readable name for `kind` (e.g. "TK_WEAK").
 * Returns "TK_UNKNOWN" for out-of-range values.
 */
const char *lexer_token_kind_name(TokenKind kind);

#endif /* CPLUS_LEXER_H */
