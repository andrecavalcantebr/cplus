#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

/*
 * FILE: test_pipeline_failure.c
 * DESC.: validates failure path for invalid source
 * AUTHOR: Andre Cavalcante
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#include "pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

static int write_text_file(const char *path, const char *content) {
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        return 0;
    }

    size_t len = strlen(content);
    size_t written = fwrite(content, 1U, len, fp);
    int close_rc = fclose(fp);

    return (written == len) && (close_rc == 0);
}

int main(void) {
    const char *input_content = "int main( { return 0; }\n";

    char input_template[] = "/tmp/cplus_invalid_XXXXXX";
    char output_template[] = "/tmp/cplus_invalid_out_XXXXXX";

    int input_fd = mkstemp(input_template);
    if (input_fd < 0) {
        fprintf(stderr, "failed to create temp input file\n");
        return 1;
    }
    close(input_fd);

    int output_fd = mkstemp(output_template);
    if (output_fd < 0) {
        fprintf(stderr, "failed to create temp output placeholder\n");
        unlink(input_template);
        return 1;
    }
    close(output_fd);

    if (unlink(output_template) != 0) {
        fprintf(stderr, "failed to prepare output path\n");
        unlink(input_template);
        unlink(output_template);
        return 1;
    }

    if (write_text_file(input_template, input_content) == 0) {
        fprintf(stderr, "failed to write input content\n");
        unlink(input_template);
        return 1;
    }

    PipelineOptions options = {
        .input_path = input_template,
        .output_path = output_template,
        .compiler = "gcc",
        .std_name = "c23",
    };

    int rc = pipeline_run(&options);

    int output_exists = (access(output_template, F_OK) == 0) ? 1 : 0;

    unlink(input_template);
    unlink(output_template);

    if (rc != 1) {
        fprintf(stderr, "expected rc=1, got rc=%d\n", rc);
        return 1;
    }

    if (output_exists != 0) {
        fprintf(stderr, "output file should not exist on validation failure\n");
        return 1;
    }

    return 0;
}
