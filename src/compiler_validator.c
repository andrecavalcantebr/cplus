#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

/*
 * FILE: compiler_validator.c
 * DESC.: syntax validation through external compilers
 * AUTHOR: Andre Cavalcante
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#include "compiler_validator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/wait.h>
#include <unistd.h>

static char *shell_quote(const char *text);
static char *read_stream_output(FILE *fp);

static int run_compiler_and_capture(
    const char *compiler,
    const char *std_name,
    const char *quoted_input,
    char **out_captured
) {
    char temp_template[] = "/tmp/cplus_diag_XXXXXX";
    int temp_fd = mkstemp(temp_template);
    if (temp_fd < 0) {
        return -1;
    }
    close(temp_fd);

    char *quoted_temp = shell_quote(temp_template);
    if (quoted_temp == NULL) {
        unlink(temp_template);
        return -1;
    }

    size_t command_size = strlen(compiler) + strlen(std_name) + strlen(quoted_input) +
                          strlen(quoted_temp) + 64U;
    char *command = (char *)malloc(command_size);
    if (command == NULL) {
        free(quoted_temp);
        unlink(temp_template);
        return -1;
    }

    (void)snprintf(
        command,
        command_size,
        "%s -x c -std=%s -fsyntax-only %s > %s 2>&1",
        compiler,
        std_name,
        quoted_input,
        quoted_temp
    );

    free(quoted_temp);

    int sys_status = system(command);
    free(command);

    FILE *diag_fp = fopen(temp_template, "rb");
    if (diag_fp == NULL) {
        unlink(temp_template);
        return -1;
    }

    char *captured = read_stream_output(diag_fp);
    (void)fclose(diag_fp);
    (void)unlink(temp_template);

    if (captured == NULL) {
        return -1;
    }

    *out_captured = captured;
    return sys_status;
}

static char *duplicate_string(const char *text) {
    size_t len = strlen(text);
    char *copy = (char *)malloc(len + 1U);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, len + 1U);
    return copy;
}

static char *shell_quote(const char *text) {
    size_t extra = 0U;

    for (const char *p = text; *p != '\0'; ++p) {
        if (*p == '\'') {
            extra += 3U;
        }
    }

    size_t len = strlen(text);
    char *quoted = (char *)malloc(len + extra + 3U);
    if (quoted == NULL) {
        return NULL;
    }

    char *out = quoted;
    *out++ = '\'';

    for (const char *p = text; *p != '\0'; ++p) {
        if (*p == '\'') {
            *out++ = '\'';
            *out++ = '\\';
            *out++ = '\'';
            *out++ = '\'';
        } else {
            *out++ = *p;
        }
    }

    *out++ = '\'';
    *out = '\0';

    return quoted;
}

static char *read_stream_output(FILE *fp) {
    size_t capacity = 4096U;
    size_t length = 0U;
    char *buffer = (char *)malloc(capacity);

    if (buffer == NULL) {
        return NULL;
    }

    for (;;) {
        if (length + 1024U + 1U > capacity) {
            size_t new_capacity = capacity * 2U;
            char *new_buffer = (char *)realloc(buffer, new_capacity);
            if (new_buffer == NULL) {
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
            capacity = new_capacity;
        }

        size_t bytes_read = fread(buffer + length, 1U, 1024U, fp);
        length += bytes_read;

        if (bytes_read < 1024U) {
            if (feof(fp) != 0) {
                break;
            }

            if (ferror(fp) != 0) {
                free(buffer);
                return NULL;
            }
        }
    }

    buffer[length] = '\0';
    return buffer;
}

ValidationResult validator_check_syntax(
    const char *compiler,
    const char *std_name,
    const char *input_path
) {
    ValidationResult result = {0, NULL};

    if ((compiler == NULL) || (std_name == NULL) || (input_path == NULL)) {
        result.raw_output = duplicate_string("error: invalid validation arguments\n");
        return result;
    }

    char *quoted_input = shell_quote(input_path);
    if (quoted_input == NULL) {
        result.raw_output = duplicate_string("error: internal runtime error (allocation failure)\n");
        return result;
    }

    char *captured = NULL;
    int sys_status = run_compiler_and_capture(compiler, std_name, quoted_input, &captured);

    if ((sys_status < 0) || (captured == NULL)) {
        result.raw_output = duplicate_string("error: failed to run compiler validation\n");
        return result;
    }

    int success = 0;
    if (WIFEXITED(sys_status) != 0) {
        success = (WEXITSTATUS(sys_status) == 0) ? 1 : 0;
    }

    if ((success == 0) && (strcmp(std_name, "c23") == 0)) {
        int has_std_option_error =
            (strstr(captured, "unrecognized command-line option") != NULL) ? 1 : 0;

        if (has_std_option_error != 0) {
            free(captured);
            captured = NULL;

            int fallback_status = run_compiler_and_capture(compiler, "c2x", quoted_input, &captured);
            if ((fallback_status >= 0) && (captured != NULL) && (WIFEXITED(fallback_status) != 0) &&
                (WEXITSTATUS(fallback_status) == 0)) {
                success = 1;
            }
        }
    }

    free(quoted_input);

    result.success = success;

    if ((captured[0] == '\0') && (success == 0)) {
        free(captured);
        result.raw_output = duplicate_string("error: compiler exited with failure and no diagnostics\n");
        return result;
    }

    result.raw_output = captured;
    return result;
}

void validator_free_result(ValidationResult *result) {
    if (result == NULL) {
        return;
    }

    free(result->raw_output);
    result->raw_output = NULL;
    result->success = 0;
}
