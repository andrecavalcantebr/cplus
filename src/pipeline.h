/*
 * FILE: pipeline.h
 * DESC.: this file is the declaration of the cplus pipeline public API
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#ifndef CPLUS_PIPELINE_H
#define CPLUS_PIPELINE_H

typedef struct {
    const char* input_path;
    const char* output_path;
    const char* compiler;   // "gcc" or "clang"
    const char* std_name;   // "c23"
} PipelineOptions;

int pipeline_run(const PipelineOptions* options);

#endif // CPLUS_PIPELINE_H
