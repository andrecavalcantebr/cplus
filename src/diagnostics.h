/*
 * FILE: diagnostics.h
 * DESC.: this file is the declaration of the cplus diagnostics model and output
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#ifndef CPLUS_DIAGNOSTICS_H
#define CPLUS_DIAGNOSTICS_H

#include <stddef.h>

/* Severity levels as emitted by GCC/Clang */
typedef enum {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_NOTE
} DiagnosticSeverity;

/*
 * A single diagnostic entry.
 * All char* fields are owned by the struct — freed by diagnostics_free_list().
 *
 * context: the raw caret/source lines that follow the primary line, or NULL.
 *          Kept verbatim so it can be replaced in v2+ when file/line is remapped.
 */
typedef struct {
    char*              file;
    int                line;
    int                column;
    DiagnosticSeverity severity;
    char*              message;
    char*              context;  /* caret + source snippet, or NULL */
} Diagnostic;

typedef struct {
    Diagnostic* items;
    size_t      count;
    size_t      capacity;
} DiagnosticList;

/* Parse raw compiler output (GCC/Clang) into a structured list */
DiagnosticList diagnostics_parse(const char* raw_output);

/* Print all diagnostics (with context) to stderr */
void diagnostics_print_list(const DiagnosticList* list);

/* Free all memory owned by the list */
void diagnostics_free_list(DiagnosticList* list);

/* Print a raw string to stderr — used for internal/runtime messages */
void diagnostics_print_raw(const char* text);

#endif // CPLUS_DIAGNOSTICS_H
