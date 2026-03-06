#ifndef CPLUS_DIAGNOSTICS_H
#define CPLUS_DIAGNOSTICS_H

typedef enum {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_NOTE
} DiagnosticSeverity;

typedef struct {
    const char* file;
    int line;
    int column;
    DiagnosticSeverity severity;
    const char* message;
} Diagnostic;

void diagnostics_print_raw(const char* text);

#endif
