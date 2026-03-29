/*
 * FILE: main.c
 * DESC.: entry point for cplus v1 CLI
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#include "pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUTS 256

static void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <file.hplus|file.cplus> [...] [-o <output>] [--cc gcc|clang] [--std c23] [-T]\n",
            program_name);
    fprintf(stderr, "  -o <output>      output path; only valid with a single input file\n");
    fprintf(stderr, "  --cc             compiler to use for validation (default: gcc)\n");
    fprintf(stderr, "  --std            C standard for validation (default: c23)\n");
    fprintf(stderr, "  -T               dump token stream to stdout (or -o file) and exit\n");
    fprintf(stderr, "  --dump-tokens    alias for -T\n");
}

/* Replace .hplus -> .h and .cplus -> .c in-place.
 * Returns a newly allocated string; caller must free(). */
static char *build_default_output_path(const char *input_path) {
    const char *ext_hplus = ".hplus";
    const char *ext_cplus = ".cplus";
    const size_t ext_len  = 6U; /* both extensions are 6 chars */

    size_t input_len = strlen(input_path);

    const char *out_ext   = NULL;
    size_t      out_ext_len = 0U;

    if ((input_len > ext_len) &&
        (strcmp(input_path + input_len - ext_len, ext_hplus) == 0)) {
        out_ext     = ".h";
        out_ext_len = 2U;
    } else if ((input_len > ext_len) &&
               (strcmp(input_path + input_len - ext_len, ext_cplus) == 0)) {
        out_ext     = ".c";
        out_ext_len = 2U;
    } else {
        /* Unknown extension: append .out */
        out_ext     = ".out";
        out_ext_len = 4U;
    }

    size_t stem_len = (out_ext[1] != 'o') ? (input_len - ext_len) : input_len;
    char *output_path = (char *)malloc(stem_len + out_ext_len + 1U);
    if (output_path == NULL) {
        return NULL;
    }

    memcpy(output_path, input_path, stem_len);
    memcpy(output_path + stem_len, out_ext, out_ext_len + 1U);

    return output_path;
}

int main(int argc, char *argv[]) {
    const char  *inputs[MAX_INPUTS];
    int          n_inputs    = 0;
    const char  *output_path = NULL;
    const char  *compiler    = "gcc";
    const char  *std_name    = "c23";
    PipelineMode mode        = PIPELINE_MODE_TRANSPILE;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-o") == 0) {
            if ((i + 1) >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            output_path = argv[++i];
        } else if (strcmp(argv[i], "--cc") == 0) {
            if ((i + 1) >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            compiler = argv[++i];
        } else if (strcmp(argv[i], "--std") == 0) {
            if ((i + 1) >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            std_name = argv[++i];
        } else if ((strcmp(argv[i], "-T") == 0) || (strcmp(argv[i], "--dump-tokens") == 0)) {
            mode = PIPELINE_MODE_DUMP_TOKENS;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            if (n_inputs >= MAX_INPUTS) {
                fprintf(stderr, "error: too many input files (max %d)\n", MAX_INPUTS);
                return 1;
            }
            inputs[n_inputs++] = argv[i];
        }
    }

    if (n_inputs == 0) {
        print_usage(argv[0]);
        return 1;
    }

    if ((output_path != NULL) && (n_inputs > 1)) {
        fprintf(stderr, "error: -o cannot be used with multiple input files\n");
        return 1;
    }

    int exit_code = 0;

    for (int i = 0; i < n_inputs; ++i) {
        char *owned_output = NULL;
        const char *out    = output_path;

        /* In dump-tokens mode the output defaults to stdout (out == NULL).
         * Only derive a .c/.h output path for the full transpile mode. */
        if ((out == NULL) && (mode == PIPELINE_MODE_TRANSPILE)) {
            owned_output = build_default_output_path(inputs[i]);
            if (owned_output == NULL) {
                fprintf(stderr, "internal runtime error: failed to allocate output path\n");
                return 2;
            }
            out = owned_output;
        }

        PipelineOptions options = {
            .input_path  = inputs[i],
            .output_path = out,
            .compiler    = compiler,
            .std_name    = std_name,
            .mode        = mode,
        };

        int rc = pipeline_run(&options);
        free(owned_output);

        if (rc != 0) {
            exit_code = rc;
        }
    }

    return exit_code;
}
