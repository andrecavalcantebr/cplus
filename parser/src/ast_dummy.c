/*
    FILE: ast_dump.c
    DESCR: Printing of the AST
    AUTHOR: Andre Cavalcante
    DATE: August, 2025
    LICENSE: CC BY-SA
*/

#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

static const char* acc_str(Access a){
    switch(a){
        case ACC_PUBLIC:    return "public";
        case ACC_PROTECTED: return "protected";
        default:            return "private";
    }
}

void module_dump(const Module *m){
    printf("\n== Interfaces (%zu) ==\n", m->nifaces);
    for(size_t i=0;i<m->nifaces;i++){
        const Interface *I=&m->ifaces[i];
        printf("interface %s {\n", I->name);
        for(size_t k=0;k<I->nmethods;k++){
            const Method *me=&I->methods[k];
            printf("    %s %s(", me->ret_type, me->name);
            for(size_t p=0;p<me->nparams;p++){
                printf("%s %s%s", me->params[p].type, me->params[p].name,
                       (p+1<me->nparams)?", ":"");
            }
            printf(");\n");
        }
        printf("};\n\n");
    }

    printf("== Classes (%zu) ==\n", m->nclasses);
    for(size_t i=0;i<m->nclasses;i++){
        const Class *C=&m->classes[i];
        printf("class %s", C->name);
        if(C->base) printf(" extends %s", C->base);
        if(C->nifaces){
            printf(" implements ");
            for(size_t j=0;j<C->nifaces;j++){
                printf("%s%s", C->ifaces[j], (j+1<C->nifaces)?", ":"");
            }
        }
        printf(" {\n");

        for(size_t f=0;f<C->nfields;f++){
            const Field *fl=&C->fields[f];
            printf("    %s: %s %s;\n", acc_str(fl->access), fl->type, fl->name);
        }
        for(size_t k=0;k<C->nmethods;k++){
            const Method *me=&C->methods[k];
            printf("    %s: %s %s(", acc_str(me->access), me->ret_type, me->name);
            for(size_t p=0;p<me->nparams;p++){
                printf("%s %s%s", me->params[p].type, me->params[p].name,
                       (p+1<me->nparams)?", ":"");
            }
            printf(");\n");
        }
        printf("};\n\n");
    }
}

void module_free(Module *m){
    /* MVP: minimal free; in production you must free strings/arrays correctly */
    (void)m;
}
