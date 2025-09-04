/*
    FILE: ast.h
    DESCR: Mininum AST for Cplus
    AUTHOR: Andre Cavalcante
    DATE: August, 2025
    LICENSE: CC BY-SA
*/

#ifndef CPLUS_AST_H
#define CPLUS_AST_H

#include <stdbool.h>
#include <stddef.h>
#include <mpc.h>

void ast_transformation(mpc_ast_t *ast, const char *output_dir);

#endif
