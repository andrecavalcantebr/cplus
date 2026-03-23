#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

/*
 * FILE: test_pipeline_success.c
 * DESC.: validates successful identity pipeline behavior
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

static char *read_text_file(const char *path) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return NULL;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    long size = ftell(fp);
    if (size < 0L) {
        fclose(fp);
        return NULL;
    }

    if (fseek(fp, 0L, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }

    char *buffer = (char *)malloc((size_t)size + 1U);
    if (buffer == NULL) {
        fclose(fp);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1U, (size_t)size, fp);
    fclose(fp);

    if (read_bytes != (size_t)size) {
        free(buffer);
        return NULL;
    }

    buffer[(size_t)size] = '\0';
    return buffer;
}

int main(void) {
    const char *input_content = "int main(void) { return 0; }\n";

    char input_template[] = "/tmp/cplus_valid_XXXXXX";
    char output_template[] = "/tmp/cplus_valid_out_XXXXXX";

    int input_fd = mkstemp(input_template);
    if (input_fd < 0) {
        fprintf(stderr, "failed to create temp input file\n");
        return 1;
    }
    close(input_fd);

    int output_fd = mkstemp(output_template);
    if (output_fd < 0) {
        fprintf(stderr, "failed to create temp output file\n");
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
    if (rc != 0) {
        fprintf(stderr, "pipeline_run returned %d\n", rc);
        unlink(input_template);
        unlink(output_template);
        return 1;
    }

    char *output_content = read_text_file(output_template);
    if (output_content == NULL) {
        fprintf(stderr, "failed to read output content\n");
        unlink(input_template);
        unlink(output_template);
        return 1;
    }

    int ok = (strcmp(output_content, input_content) == 0) ? 1 : 0;
    free(output_content);

    unlink(input_template);
    unlink(output_template);

    if (ok == 0) {
        fprintf(stderr, "output content mismatch\n");
        return 1;
    }

    return 0;
}
