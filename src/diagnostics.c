/*
 * FILE: diagnostics.c
 * DESC.: diagnostics output utilities for cplus
 * AUTHOR: Andre Cavalcante
 * LICENSE: GPL-v3
 * DATE: March, 2026
 */

#include "diagnostics.h"

#include <stdio.h>

void diagnostics_print_raw(const char *text) {
    if (text == NULL) {
        return;
    }

    fputs(text, stderr);
}
