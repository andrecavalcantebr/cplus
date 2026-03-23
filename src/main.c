/*
 * FILE: main.c
 * DESC.: entry point for cplus v1 CLI
 * AUTHOR: Andre Cavalcante
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#include "pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <input.c> [-o <output.c>] [--cc gcc|clang] [--std c23]\n", program_name);
}

static char *build_default_output_path(const char *input_path) {
    const char suffix[] = ".out.c";
    size_t input_len = strlen(input_path);
    size_t suffix_len = sizeof(suffix) - 1U;

    char *output_path = (char *)malloc(input_len + suffix_len + 1U);
    if (output_path == NULL) {
        return NULL;
    }

    memcpy(output_path, input_path, input_len);
    memcpy(output_path + input_len, suffix, suffix_len + 1U);

    return output_path;
}

int main(int argc, char *argv[]) {
    const char *input_path = NULL;
    const char *output_path = NULL;
    const char *compiler = "gcc";
    const char *std_name = "c23";
    char *owned_default_output = NULL;

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
        } else if (argv[i][0] == '-') {
            print_usage(argv[0]);
            return 1;
        } else if (input_path == NULL) {
            input_path = argv[i];
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (input_path == NULL) {
        print_usage(argv[0]);
        return 1;
    }

    if (output_path == NULL) {
        owned_default_output = build_default_output_path(input_path);
        if (owned_default_output == NULL) {
            fprintf(stderr, "internal runtime error: failed to allocate output path\n");
            return 2;
        }
        output_path = owned_default_output;
    }

    PipelineOptions options = {
        .input_path = input_path,
        .output_path = output_path,
        .compiler = compiler,
        .std_name = std_name,
    };

    int rc = pipeline_run(&options);
    free(owned_default_output);
    return rc;
}
