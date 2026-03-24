/*
 * FILE: compiler_validator.h
 * DESC.: this file is the declaration of the compiler syntax validation API
 * AUTHOR: Andre Cavalcante and Claude Sonnet 4.6 as pair programmer
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#ifndef CPLUS_COMPILER_VALIDATOR_H
#define CPLUS_COMPILER_VALIDATOR_H

typedef struct {
    int success;      // 1 if syntax is valid, 0 otherwise
    char* raw_output; // compiler stderr/stdout capture
} ValidationResult;

ValidationResult validator_check_syntax(
    const char* compiler,
    const char* std_name,
    const char* input_path
);

void validator_free_result(ValidationResult* result);

#endif // CPLUS_COMPILER_VALIDATOR_H
