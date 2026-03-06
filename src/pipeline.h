#ifndef CPLUS_PIPELINE_H
#define CPLUS_PIPELINE_H

typedef struct {
    const char* input_path;
    const char* output_path;
    const char* compiler;   // "gcc" or "clang"
    const char* std_name;   // "c23"
} PipelineOptions;

int pipeline_run(const PipelineOptions* options);

#endif
