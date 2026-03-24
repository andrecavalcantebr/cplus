#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

/*
 * FILE: test_golden_valid_clang.c
 * DESC.: golden test — valid C23 input produces identical output via clang (identity transform)
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#include "pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#ifndef CPLUS_FIXTURES_DIR
#error "CPLUS_FIXTURES_DIR must be defined at compile time"
#endif

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
    const char *fixture_input    = CPLUS_FIXTURES_DIR "/valid_simple.cplus";
    const char *fixture_expected = CPLUS_FIXTURES_DIR "/valid_simple.expected.c";

    char output_template[] = "/tmp/cplus_golden_valid_clang_XXXXXX";
    int output_fd = mkstemp(output_template);
    if (output_fd < 0) {
        fprintf(stderr, "failed to create temp output file\n");
        return 1;
    }
    close(output_fd);

    /* Remove the placeholder so the pipeline creates it fresh */
    if (unlink(output_template) != 0) {
        fprintf(stderr, "failed to remove temp output placeholder\n");
        return 1;
    }

    PipelineOptions options = {
        .input_path  = fixture_input,
        .output_path = output_template,
        .compiler    = "clang",
        .std_name    = "c23",
    };

    int rc = pipeline_run(&options);

    if (rc != 0) {
        fprintf(stderr, "pipeline_run returned %d, expected 0\n", rc);
        unlink(output_template);
        return 1;
    }

    char *actual   = read_text_file(output_template);
    char *expected = read_text_file(fixture_expected);

    unlink(output_template);

    if (actual == NULL) {
        fprintf(stderr, "failed to read pipeline output\n");
        free(expected);
        return 1;
    }

    if (expected == NULL) {
        fprintf(stderr, "failed to read expected fixture: %s\n", fixture_expected);
        free(actual);
        return 1;
    }

    int match = (strcmp(actual, expected) == 0);

    if (match == 0) {
        fprintf(stderr, "output does not match expected fixture\n");
        fprintf(stderr, "--- expected ---\n%s\n--- actual ---\n%s\n", expected, actual);
    }

    free(actual);
    free(expected);

    return (match != 0) ? 0 : 1;
}
