/*
 * FILE: pipeline.h
 * DESC.: this file is the declaration of the cplus pipeline public API
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#ifndef CPLUS_PIPELINE_H
#define CPLUS_PIPELINE_H

/*
 * PipelineMode — selects the operating mode for pipeline_run().
 *
 *   PIPELINE_MODE_TRANSPILE   — full transpile: validate + lower + write output
 *   PIPELINE_MODE_DUMP_TOKENS — lex only: print the token stream, no output file
 */
typedef enum {
    PIPELINE_MODE_TRANSPILE   = 0,
    PIPELINE_MODE_DUMP_TOKENS
} PipelineMode;

typedef struct {
    const char  *input_path;
    const char  *output_path; /* NULL or "-" → stdout (dump-tokens mode only) */
    const char  *compiler;    /* "gcc" or "clang"                              */
    const char  *std_name;    /* "c23"                                         */
    PipelineMode mode;
} PipelineOptions;

int pipeline_run(const PipelineOptions* options);

#endif // CPLUS_PIPELINE_H
