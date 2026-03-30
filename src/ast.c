/*
 * FILE: ast.c
 * DESC.: implementation of the sparse island AST for cplus v2.  Provides
 *        constructors, recursive free, and a debug dump utility.
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------- */
/* Internal helpers                                                         */
/* ---------------------------------------------------------------------- */

/*
 * alloc_node — allocate a zeroed AstNode and set the common header fields.
 * Returns NULL on OOM.
 */
static AstNode *alloc_node(NodeKind kind, int line, int col) {
    AstNode *n = calloc(1, sizeof(AstNode));
    if (n == NULL) {
        return NULL;
    }
    n->kind = kind;
    n->line = line;
    n->col  = col;
    return n;
}

/* ---------------------------------------------------------------------- */
/* Constructors                                                             */
/* ---------------------------------------------------------------------- */

AstNode *ast_passthrough(const char *text, size_t len, int line, int col) {
    AstNode *n = alloc_node(NODE_PASSTHROUGH, line, col);
    if (n == NULL) {
        return NULL;
    }
    n->passthrough.text = strndup(text, len);
    if (n->passthrough.text == NULL) {
        free(n);
        return NULL;
    }
    n->passthrough.len = len;
    return n;
}

AstNode *ast_class(const char *name, AstNode *fields, int line, int col) {
    AstNode *n = alloc_node(NODE_CLASS, line, col);
    if (n == NULL) {
        return NULL;
    }
    n->class_decl.name = strdup(name);
    if (n->class_decl.name == NULL) {
        free(n);
        return NULL;
    }
    n->class_decl.fields = fields; /* ownership transferred */
    return n;
}

AstNode *ast_weak_decl(const char *type_ref, const char *name,
                       const char *init_text, int line, int col) {
    AstNode *n = alloc_node(NODE_WEAK_DECL, line, col);
    if (n == NULL) {
        return NULL;
    }

    n->weak_decl.type_ref = strdup(type_ref);
    if (n->weak_decl.type_ref == NULL) {
        free(n);
        return NULL;
    }

    n->weak_decl.name = strdup(name);
    if (n->weak_decl.name == NULL) {
        free(n->weak_decl.type_ref);
        free(n);
        return NULL;
    }

    if (init_text != NULL) {
        n->weak_decl.init_text = strdup(init_text);
        if (n->weak_decl.init_text == NULL) {
            free(n->weak_decl.name);
            free(n->weak_decl.type_ref);
            free(n);
            return NULL;
        }
    }

    return n;
}

AstNode *ast_unique_decl(const char *destructor, const char *type_ref,
                         const char *name, AstNode *init, int line, int col) {
    AstNode *n = alloc_node(NODE_UNIQUE_DECL, line, col);
    if (n == NULL) {
        return NULL;
    }

    if (destructor != NULL) {
        n->unique_decl.destructor = strdup(destructor);
        if (n->unique_decl.destructor == NULL) {
            free(n);
            return NULL;
        }
    }

    n->unique_decl.type_ref = strdup(type_ref);
    if (n->unique_decl.type_ref == NULL) {
        free(n->unique_decl.destructor);
        free(n);
        return NULL;
    }

    n->unique_decl.name = strdup(name);
    if (n->unique_decl.name == NULL) {
        free(n->unique_decl.type_ref);
        free(n->unique_decl.destructor);
        free(n);
        return NULL;
    }

    n->unique_decl.init = init; /* ownership transferred */
    return n;
}

AstNode *ast_move_expr(const char *source, int line, int col) {
    AstNode *n = alloc_node(NODE_MOVE_EXPR, line, col);
    if (n == NULL) {
        return NULL;
    }
    n->move_expr.source = strdup(source);
    if (n->move_expr.source == NULL) {
        free(n);
        return NULL;
    }
    return n;
}

/* ---------------------------------------------------------------------- */
/* Lifecycle                                                                */
/* ---------------------------------------------------------------------- */

void ast_free(AstNode *node) {
    while (node != NULL) {
        AstNode *next = node->next; /* save before freeing */

        switch (node->kind) {
            case NODE_PASSTHROUGH:
                free(node->passthrough.text);
                break;

            case NODE_CLASS:
                free(node->class_decl.name);
                ast_free(node->class_decl.fields); /* recurse into children */
                break;

            case NODE_WEAK_DECL:
                free(node->weak_decl.type_ref);
                free(node->weak_decl.name);
                free(node->weak_decl.init_text);   /* NULL-safe via free() */
                break;

            case NODE_UNIQUE_DECL:
                free(node->unique_decl.destructor);
                free(node->unique_decl.type_ref);
                free(node->unique_decl.name);
                ast_free(node->unique_decl.init);  /* recurse into init child */
                break;

            case NODE_MOVE_EXPR:
                free(node->move_expr.source);
                break;
        }

        free(node);
        node = next;
    }
}

/* ---------------------------------------------------------------------- */
/* Introspection / debug                                                    */
/* ---------------------------------------------------------------------- */

const char *ast_node_kind_name(NodeKind kind) {
    switch (kind) {
        case NODE_PASSTHROUGH:  return "PASSTHROUGH";
        case NODE_CLASS:        return "CLASS";
        case NODE_WEAK_DECL:    return "WEAK_DECL";
        case NODE_UNIQUE_DECL:  return "UNIQUE_DECL";
        case NODE_MOVE_EXPR:    return "MOVE_EXPR";
    }
    return "NODE_UNKNOWN";
}

/*
 * print_indent — emit `depth * 2` spaces to `out`.
 */
static void print_indent(int depth, FILE *out) {
    for (int i = 0; i < depth; ++i) {
        fputs("  ", out);
    }
}

void ast_dump(const AstNode *node, int depth, FILE *out) {
    for (const AstNode *n = node; n != NULL; n = n->next) {
        print_indent(depth, out);

        switch (n->kind) {
            case NODE_PASSTHROUGH:
                fprintf(out, "PASSTHROUGH len=%zu line=%d:%d\n",
                        n->passthrough.len, n->line, n->col);
                break;

            case NODE_CLASS:
                fprintf(out, "CLASS \"%s\" line=%d:%d\n",
                        n->class_decl.name, n->line, n->col);
                ast_dump(n->class_decl.fields, depth + 1, out);
                break;

            case NODE_WEAK_DECL: {
                const char *init = n->weak_decl.init_text;
                if (init != NULL) {
                    fprintf(out, "WEAK_DECL type=\"%s\" name=\"%s\" init=\"%s\" line=%d:%d\n",
                            n->weak_decl.type_ref, n->weak_decl.name,
                            init, n->line, n->col);
                } else {
                    fprintf(out, "WEAK_DECL type=\"%s\" name=\"%s\" line=%d:%d\n",
                            n->weak_decl.type_ref, n->weak_decl.name,
                            n->line, n->col);
                }
                break;
            }

            case NODE_UNIQUE_DECL: {
                const char *dtor = n->unique_decl.destructor;
                fprintf(out, "UNIQUE_DECL type=\"%s\" name=\"%s\" dtor=%s%s%s line=%d:%d\n",
                        n->unique_decl.type_ref,
                        n->unique_decl.name,
                        dtor ? "\"" : "(default)",
                        dtor ? dtor : "",
                        dtor ? "\""  : "",
                        n->line, n->col);
                ast_dump(n->unique_decl.init, depth + 1, out);
                break;
            }

            case NODE_MOVE_EXPR:
                fprintf(out, "MOVE_EXPR source=\"%s\" line=%d:%d\n",
                        n->move_expr.source, n->line, n->col);
                break;
        }
    }
}
