/*
 * FILE: pipeline.c
 * DESC.: high-level workflow for cplus v1
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#include "pipeline.h"

#include "compiler_validator.h"
#include "diagnostics.h"

#include <stdio.h>
#include <stdlib.h>

static char *read_entire_file(const char *path, size_t *out_size) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return NULL;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    long file_size = ftell(fp);
    if (file_size < 0L) {
        fclose(fp);
        return NULL;
    }

    if (fseek(fp, 0L, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }

    size_t size = (size_t)file_size;
    char *buffer = (char *)malloc(size + 1U);
    if (buffer == NULL) {
        fclose(fp);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1U, size, fp);
    fclose(fp);

    if (read_bytes != size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    if (out_size != NULL) {
        *out_size = size;
    }

    return buffer;
}

static int write_entire_file(const char *path, const char *data, size_t size) {
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        return 0;
    }

    size_t written = fwrite(data, 1U, size, fp);
    int close_rc = fclose(fp);

    if ((written != size) || (close_rc != 0)) {
        return 0;
    }

    return 1;
}

int pipeline_run(const PipelineOptions *options) {
    if ((options == NULL) || (options->input_path == NULL) || (options->output_path == NULL) ||
        (options->compiler == NULL) || (options->std_name == NULL)) {
        diagnostics_print_raw("error: invalid pipeline options\n");
        return 1;
    }

    ValidationResult validation = validator_check_syntax(
        options->compiler,
        options->std_name,
        options->input_path
    );

    if (validation.success == 0) {
        DiagnosticList diags = diagnostics_parse(validation.raw_output);
        if (diags.count > 0U) {
            diagnostics_print_list(&diags);
        } else {
            /* Fallback: compiler output didn't match expected format */
            diagnostics_print_raw(validation.raw_output);
        }
        diagnostics_free_list(&diags);
        validator_free_result(&validation);
        return 1;
    }

    validator_free_result(&validation);

    size_t input_size = 0U;
    char *source = read_entire_file(options->input_path, &input_size);
    if (source == NULL) {
        diagnostics_print_raw("error: failed to read input file\n");
        return 1;
    }

    int write_ok = write_entire_file(options->output_path, source, input_size);
    free(source);

    if (write_ok == 0) {
        diagnostics_print_raw("error: failed to write output file\n");
        return 1;
    }

    return 0;
}
