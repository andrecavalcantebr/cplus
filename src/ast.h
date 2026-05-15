/*
 * FILE: ast.h
 * DESC.: sparse island AST for cplus v2.  Only cplus-specific constructs are
 *        represented as typed nodes; all other C23 text is captured verbatim
 *        in NODE_PASSTHROUGH nodes.  Every string stored inside a node is
 *        heap-allocated and owned by the node.  Callers must use ast_free()
 *        to release the tree.
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#ifndef CPLUS_AST_H
#define CPLUS_AST_H

#include <stddef.h>
#include <stdio.h>

/* ---------------------------------------------------------------------- */
/* Node kinds                                                               */
/* ---------------------------------------------------------------------- */

typedef enum {
    NODE_PASSTHROUGH,   /* verbatim C23 text — not a cplus construct        */
    NODE_STRUCT_DECL,   /* struct Name { fields } at file scope             */
    NODE_WEAK_DECL,     /* weak T *name [= expr]                            */
    NODE_UNIQUE_DECL,   /* unique[(dtor)] T *name = init                    */
    NODE_MOVE_EXPR      /* move(source) — child of NODE_UNIQUE_DECL.init    */
} NodeKind;

/* ---------------------------------------------------------------------- */
/* AstNode — tagged union                                                   */
/* ---------------------------------------------------------------------- */

/*
 * Forward declaration so that the struct members can reference the type
 * recursively.
 */
typedef struct AstNode AstNode;

/*
 * A single AST node.
 *
 * `next` forms a singly-linked list of sibling nodes (e.g. the top-level
 * statement sequence of a translation unit, or the field list of a class).
 * The caller that builds the list is responsible for chaining `next`; all
 * constructor functions return a standalone node with `next == NULL`.
 *
 * `line` and `col` refer to the first byte of the construct in the source
 * file (1-based).  They are used for diagnostic messages.
 *
 * All heap-allocated strings inside the union are owned by this node and
 * freed by ast_free().
 */
struct AstNode {
    NodeKind  kind;
    int       line;
    int       col;
    AstNode  *next;   /* next sibling; NULL = end of list                  */

    union {
        /*
         * NODE_PASSTHROUGH
         * Verbatim C23 text copied from the source buffer.
         * `text` is a heap-allocated, NUL-terminated copy.
         */
        struct {
            char  *text;
            size_t len;
        } passthrough;

        /*
         * NODE_STRUCT_DECL
         * struct Name { fields } — at file scope only; auto-typedef emitted.
         *
         * `name`   — heap-allocated identifier string.
         * `fields` — linked list of child NODE_PASSTHROUGH nodes (field text).
         *            May be NULL for an empty body.
         */
        struct {
            char    *name;
            AstNode *fields;  /* first child; walk via ->next              */
        } struct_decl;

        /*
         * NODE_WEAK_DECL
         * weak T *name [= init_text]
         *
         * `type_ref`  — heap-allocated type token(s), e.g. "int", "Person".
         * `name`      — heap-allocated declarator identifier.
         * `init_text` — heap-allocated initializer expression text, or NULL
         *               when no initializer is present.
         */
        struct {
            char *type_ref;
            char *name;
            char *init_text;  /* NULL if no initializer                    */
        } weak_decl;

        /*
         * NODE_UNIQUE_DECL
         * unique[(destructor)] T *name = init
         *
         * `destructor` — heap-allocated destructor name, or NULL when the
         *                default (`free`) is used.
         * `type_ref`   — heap-allocated type token(s).
         * `name`       — heap-allocated declarator identifier.
         * `init`       — child node representing the initializer:
         *                  NODE_MOVE_EXPR  when the init is move(src)
         *                  NODE_PASSTHROUGH when the init is an expression
         */
        struct {
            char    *destructor; /* NULL → default free                    */
            char    *type_ref;
            char    *name;
            AstNode *init;       /* owned child; never NULL (E202 if absent) */
        } unique_decl;

        /*
         * NODE_MOVE_EXPR
         * move(source)
         *
         * `source` — heap-allocated name of the variable being moved.
         */
        struct {
            char *source;
        } move_expr;
    };
};

/* ---------------------------------------------------------------------- */
/* Constructors                                                             */
/* ---------------------------------------------------------------------- */

/*
 * All constructors return a heap-allocated AstNode, or NULL on OOM.
 * The returned node has `next == NULL`.  The caller is responsible for
 * linking siblings via the `next` pointer.
 *
 * String arguments are copied with strdup / strndup — the AST does not
 * retain any pointer into the original source buffer.
 */

/*
 * ast_passthrough — capture verbatim C23 text.
 * `text` must point to `len` bytes; it need not be NUL-terminated.
 */
AstNode *ast_passthrough(const char *text, size_t len, int line, int col);

/*
 * ast_struct_decl — struct Name { fields } at file scope
 * `name`   — NUL-terminated identifier.
 * `fields` — head of the child field list (may be NULL).
 *            Ownership is transferred to the new node.
 */
AstNode *ast_struct_decl(const char *name, AstNode *fields, int line, int col);

/*
 * ast_weak_decl — weak T *name [= init_text]
 * `type_ref`  — NUL-terminated type string.
 * `name`      — NUL-terminated declarator identifier.
 * `init_text` — NUL-terminated initializer expression, or NULL.
 */
AstNode *ast_weak_decl(const char *type_ref, const char *name,
                       const char *init_text, int line, int col);

/*
 * ast_unique_decl — unique[(destructor)] T *name = init
 * `destructor` — NUL-terminated destructor name, or NULL for the default.
 * `type_ref`   — NUL-terminated type string.
 * `name`       — NUL-terminated declarator identifier.
 * `init`       — initializer child node; ownership is transferred.
 *                Must not be NULL (callers must emit E202 before calling).
 */
AstNode *ast_unique_decl(const char *destructor, const char *type_ref,
                         const char *name, AstNode *init, int line, int col);

/*
 * ast_move_expr — move(source)
 * `source` — NUL-terminated name of the variable being moved.
 */
AstNode *ast_move_expr(const char *source, int line, int col);

/* ---------------------------------------------------------------------- */
/* Lifecycle                                                                */
/* ---------------------------------------------------------------------- */

/*
 * ast_free — recursively free a node and all reachable nodes (children and
 * siblings via `next`).  NULL-safe.
 */
void ast_free(AstNode *node);

/* ---------------------------------------------------------------------- */
/* Introspection / debug                                                    */
/* ---------------------------------------------------------------------- */

/*
 * ast_node_kind_name — return a stable human-readable name for `kind`.
 * Returns "NODE_UNKNOWN" for out-of-range values.
 */
const char *ast_node_kind_name(NodeKind kind);

/*
 * ast_dump — print the tree rooted at `node` to `out` in a human-readable
 * indented format.  `depth` controls the initial indentation level (pass 0
 * for the root).  Siblings reachable via `next` are all printed at the same
 * depth.
 *
 * Output example:
 *   PASSTHROUGH len=42 line=1:1
 *   STRUCT_DECL "Point" line=1:1
 *     PASSTHROUGH len=12 line=2:5
 *   UNIQUE_DECL type="Person" name="alice" dtor="person_destroy" line=10:5
 *     MOVE_EXPR source="tmp" line=10:35
 *   WEAK_DECL type="int" name="x" init="malloc(4)" line=15:1
 */
void ast_dump(const AstNode *node, int depth, FILE *out);

#endif /* CPLUS_AST_H */
