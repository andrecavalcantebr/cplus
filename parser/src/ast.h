/*
    FILE: ast.h
    DESCR: Mininum AST for Cplus headers
    AUTHOR: Andre Cavalcante
    DATE: August, 2025
    LICENSE: CC BY-SA
*/

#ifndef CPLUS_AST_H
#define CPLUS_AST_H

#include <stdbool.h>
#include <stddef.h>
#include <mpc.h>

typedef enum
{
    ACC_PUBLIC,
    ACC_PROTECTED,
    ACC_PRIVATE
} Access;

typedef struct
{
    char *type; /* text for the type */
    char *name; /* identifier */
} Param;

typedef struct
{
    char *ret_type; /* text to return type */
    char *name;     /* ex.: Motor_power_on */
    Param *params;  /* array */
    size_t nparams;
    Access access;
} Method;

typedef struct
{
    char *type; /* text of type */
    char *name; /* identifier */
    Access access;
} Field;

typedef struct
{
    char *name;
    Method *methods;
    size_t nmethods;
} Interface;

typedef struct
{
    char *name;
    char *base;    /* extends (optional) */
    char **ifaces; /* implements */
    size_t nifaces;

    Field *fields;
    size_t nfields;
    Method *methods;
    size_t nmethods;
} Class;

typedef struct
{
    Interface *ifaces;
    size_t nifaces;
    Class *classes;
    size_t nclasses;
} Module;

/* util */
void module_free(Module *m);
void module_dump(const Module *m);

void print_program(const mpc_ast_t *root);

#endif
