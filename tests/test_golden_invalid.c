#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

/*
 * FILE: test_golden_invalid.c
 * DESC.: golden test — invalid C23 input causes rc=1 and no output file
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#include "pipeline.h"

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#ifndef CPLUS_FIXTURES_DIR
#error "CPLUS_FIXTURES_DIR must be defined at compile time"
#endif

int main(void) {
    const char *fixture_input = CPLUS_FIXTURES_DIR "/invalid_syntax.cplus";

    char output_template[] = "/tmp/cplus_golden_invalid_XXXXXX";
    int output_fd = mkstemp(output_template);
    if (output_fd < 0) {
        fprintf(stderr, "failed to create temp output placeholder\n");
        return 1;
    }
    close(output_fd);

    /* Remove the placeholder so any accidental creation is detectable */
    if (unlink(output_template) != 0) {
        fprintf(stderr, "failed to remove temp output placeholder\n");
        return 1;
    }

    PipelineOptions options = {
        .input_path  = fixture_input,
        .output_path = output_template,
        .compiler    = "gcc",
        .std_name    = "c23",
    };

    int rc = pipeline_run(&options);

    int output_exists = (access(output_template, F_OK) == 0) ? 1 : 0;

    /* Clean up in case the pipeline incorrectly wrote a file */
    if (output_exists != 0) {
        unlink(output_template);
    }

    if (rc != 1) {
        fprintf(stderr, "expected rc=1 for invalid input, got rc=%d\n", rc);
        return 1;
    }

    if (output_exists != 0) {
        fprintf(stderr, "output file must not be created when validation fails\n");
        return 1;
    }

    return 0;
}
