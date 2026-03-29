/*
 * FILE: diagnostics.c
 * DESC.: diagnostics output utilities for cplus
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 *
 * NOTE: strdup / strndup are standard C23 (ISO/IEC 9899:2024 §7.27.6).
 *       No POSIX feature-test macro is required when building with -std=c23.
 */

#include "diagnostics.h"
#include "strbuf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/*
 * Try to parse a primary diagnostic line of the form:
 *   <file>:<line>:<col>: <severity>: <message>
 *
 * line_start / line_end delimit the line slice (line_end is exclusive,
 * no NUL assumed at line_end). No intermediate copies are made — on
 * success the out-pointers point directly into raw_output and the caller
 * uses strndup (C23) to build owned strings.
 *
 * Returns 1 on match, 0 otherwise.
 */
static int try_parse_primary(
    const char *line_start, const char *line_end,
    const char **file_p,  size_t *file_sz,
    int         *out_line, int   *out_col,
    DiagnosticSeverity    *out_sev,
    const char **msg_p,   size_t *msg_sz
) {
    const char *p = line_start;

    /* --- file: up to the first ':' followed by a digit --- */
    const char *colon1 = NULL;
    for (const char *q = p; q + 1 < line_end; ++q) {
        if (*q == ':' && q[1] >= '0' && q[1] <= '9') {
            colon1 = q;
            break;
        }
    }
    if (colon1 == NULL || colon1 == p) {
        return 0;
    }

    /* --- line number --- */
    char *end_ln = NULL;
    long parsed_ln = strtol(colon1 + 1, &end_ln, 10);
    if (end_ln == colon1 + 1 || end_ln >= line_end || *end_ln != ':') {
        return 0;
    }

    /* --- column number --- */
    char *end_col = NULL;
    long parsed_col = strtol(end_ln + 1, &end_col, 10);
    if (end_col == end_ln + 1 || end_col + 1 >= line_end ||
        end_col[0] != ':' || end_col[1] != ' ') {
        return 0;
    }

    /* --- severity: known token before ": " --- */
    const char *sev_start = end_col + 2;
    const char *sev_colon = NULL;
    for (const char *q = sev_start; q + 1 < line_end; ++q) {
        if (*q == ':' && q[1] == ' ') {
            sev_colon = q;
            break;
        }
    }
    if (sev_colon == NULL) {
        return 0;
    }

    size_t sev_len = (size_t)(sev_colon - sev_start);
    DiagnosticSeverity sev;
    if        (sev_len == 5U && memcmp(sev_start, "error",   5U) == 0) {
        sev = DIAG_ERROR;
    } else if (sev_len == 7U && memcmp(sev_start, "warning", 7U) == 0) {
        sev = DIAG_WARNING;
    } else if (sev_len == 4U && memcmp(sev_start, "note",    4U) == 0) {
        sev = DIAG_NOTE;
    } else {
        return 0;
    }

    /* --- message: everything after "severity: " --- */
    const char *msg_start = sev_colon + 2;
    if (msg_start >= line_end) {
        return 0;
    }

    *file_p   = p;
    *file_sz  = (size_t)(colon1 - p);
    *out_line = (int)parsed_ln;
    *out_col  = (int)parsed_col;
    *out_sev  = sev;
    *msg_p    = msg_start;
    *msg_sz   = (size_t)(line_end - msg_start);
    return 1;
}

/* Add a completed Diagnostic to the list, growing as needed. */
static int list_push(DiagnosticList *list, Diagnostic *diag) {
    if (list->count >= list->capacity) {
        size_t new_cap = (list->capacity == 0U) ? 8U : list->capacity * 2U;
        Diagnostic *resized =
            (Diagnostic *)realloc(list->items, new_cap * sizeof(Diagnostic));
        if (resized == NULL) {
            return 0;
        }
        list->items    = resized;
        list->capacity = new_cap;
    }
    list->items[list->count++] = *diag;
    return 1;
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

DiagnosticList diagnostics_parse(const char *raw_output) {
    DiagnosticList list = {NULL, 0U, 0U};

    if (raw_output == NULL || raw_output[0] == '\0') {
        return list;
    }

    /*
     * State machine — walk raw_output line by line without copying it.
     *
     *   current.file != NULL  → inside a diagnostic, accumulating context
     *   current.file == NULL  → idle, waiting for a primary line
     */
    Diagnostic current = {NULL, 0, 0, DIAG_ERROR, NULL, NULL};
    StrBuf ctx;
    strbuf_init(&ctx);

    const char *pos = raw_output;

    while (*pos != '\0') {
        const char *nl       = strchr(pos, '\n');
        const char *line_end = (nl != NULL) ? nl : (pos + strlen(pos));
        size_t      line_len = (size_t)(line_end - pos);

        const char        *file_p  = NULL;
        const char        *msg_p   = NULL;
        size_t             file_sz = 0U;
        size_t             msg_sz  = 0U;
        int                ln_num  = 0;
        int                col_num = 0;
        DiagnosticSeverity sev     = DIAG_ERROR;

        int matched = try_parse_primary(
            pos, line_end,
            &file_p, &file_sz,
            &ln_num, &col_num, &sev,
            &msg_p,  &msg_sz
        );

        if (matched != 0) {
            /* Flush previous diagnostic */
            if (current.file != NULL) {
                current.context = strbuf_take(&ctx, NULL);
                (void)list_push(&list, &current);
                current = (Diagnostic){NULL, 0, 0, DIAG_ERROR, NULL, NULL};
                strbuf_init(&ctx);
            }

            /* strndup: C23 standard (§7.27.6) — no copy into fixed buffer */
            current.file     = strndup(file_p, file_sz);
            current.message  = strndup(msg_p,  msg_sz);
            current.line     = ln_num;
            current.column   = col_num;
            current.severity = sev;

            if (current.file == NULL || current.message == NULL) {
                free(current.file);
                free(current.message);
                break;
            }
        } else {
            /* Context line (caret, source snippet, or "In function" header) */
            if (current.file != NULL && line_len > 0U) {
                (void)strbuf_append_bytes(&ctx, pos, line_len);
                (void)strbuf_append_cstr(&ctx, "\n");
            }
            /* Lines before the first diagnostic are dropped */
        }

        pos = (nl != NULL) ? nl + 1 : line_end;
    }

    /* Flush last pending diagnostic */
    if (current.file != NULL) {
        current.context = strbuf_take(&ctx, NULL);
        (void)list_push(&list, &current);
    } else {
        strbuf_free(&ctx);
    }

    return list;
}

void diagnostics_print_list(const DiagnosticList *list) {
    if (list == NULL) {
        return;
    }

    static const char *const severity_names[] = {
        [DIAG_ERROR]   = "error",
        [DIAG_WARNING] = "warning",
        [DIAG_NOTE]    = "note",
    };

    for (size_t i = 0U; i < list->count; ++i) {
        const Diagnostic *d = &list->items[i];
        fprintf(stderr, "%s:%d:%d: %s: %s\n",
                d->file, d->line, d->column,
                severity_names[d->severity],
                d->message);
        if (d->context != NULL) {
            fputs(d->context, stderr);
        }
    }
}

void diagnostics_free_list(DiagnosticList *list) {
    if (list == NULL) {
        return;
    }

    for (size_t i = 0U; i < list->count; ++i) {
        free(list->items[i].file);
        free(list->items[i].message);
        free(list->items[i].context);
    }

    free(list->items);
    list->items    = NULL;
    list->count    = 0U;
    list->capacity = 0U;
}

void diagnostics_print_raw(const char *text) {
    if (text == NULL) {
        return;
    }

    fputs(text, stderr);
}
