/*
    FILE: main.c
    DESCR: Driver do parser — read a .h Cplus and prints the AST
    AUTHOR: Andre Cavalcante
    DATE: August, 2025
    LICENSE: CC BY-SA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpc.h>

#include "ast.h"

/* parser declaration */
Module parse_cplus(const char *input);

static char* slurp(const char *path){
    FILE *f=fopen(path,"rb"); 
    if(!f) return NULL;
    fseek(f,0,SEEK_END); 
    long n=ftell(f); 
    rewind(f);
    char *buf=malloc((size_t)n+1);
    if(!buf) { 
        fclose(f); 
        return NULL; 
    }
    fread(buf,1,(size_t)n,f); 
    buf[n]='\0'; 
    fclose(f); 
    return buf;
}

/* Remove //... e / * ... * / , preservando quebras de linha. */
static char* strip_comments(const char *s) {
    size_t n = strlen(s), cap = n + 1;
    char *out = malloc(cap);
    if (!out) return NULL;

    enum { NORM, LINE, BLOCK } st = NORM;
    size_t i=0, o=0;
    while (i < n) {
        char c = s[i], nxt = (i+1<n ? s[i+1] : '\0');

        if (o + 2 >= cap) { cap *= 2; out = realloc(out, cap); if(!out) return NULL; }

        switch (st) {
        case NORM:
            if (c == '/' && nxt == '/') { st = LINE; i += 2; continue; }
            if (c == '/' && nxt == '*') { st = BLOCK; i += 2; continue; }
            out[o++] = c; i++; break;

        case LINE: /* // até \n */
            if (c == '\n') { out[o++] = '\n'; st = NORM; }
            i++; break;

        case BLOCK: /* até */
            if (c == '*' && nxt == '/') { st = NORM; i += 2; continue; }
            if (c == '\n') out[o++] = '\n';
            i++; break;
        }
    }
    out[o] = '\0';
    return out;
}


int main(int argc, char **argv){
    if (argc<2) { 
        fprintf(stderr, "uso: %s <arquivo.cplus.h>\n", argv[0]); 
        return 2; 
    }

    char *txt = slurp(argv[1]); 
    if(!txt) { 
        perror("slurp"); 
        return 1; 
    }

    char *clean = strip_comments(txt);
    free(txt);
    if (!clean) { 
        fprintf(stderr, "mem\n"); 
        return 1; 
    }

    Module m = parse_cplus(clean);
    module_dump(&m);
    free(clean);
    module_free(&m);
    
    return 0;
}


/* Concatena tokens de um typespec (ident, '*', '[]', qualifiers) com espaços */
static char* grab_typespec(const mpc_ast_t *ts) {
    size_t cap = 64, len = 0;
    char *buf = (char*)malloc(cap);
    if (!buf) return NULL;
    buf[0] = '\0';

    for (int i = 0; i < ts->children_num; i++) {
        const mpc_ast_t *c = ts->children[i];
        if (strstr(c->tag, "ident")
         || strstr(c->tag, "ptr")
         || strstr(c->tag, "qual")
         || strstr(c->tag, "lbrack")
         || strstr(c->tag, "rbrack")) {

            const char *tok = c->contents;
            size_t add = strlen(tok) + 1; /* + espaço */
            if (len + add + 1 > cap) { cap *= 2; buf = (char*)realloc(buf, cap); }
            if (!buf) return NULL;

            if (len) buf[len++] = ' ';
            strcpy(buf + len, tok);
            len += strlen(tok);
        }
    }
    return buf;
}

/* ============================================================
 * Parse for AST (Module)
 * ============================================================ */

Module parse_cplus(const char *input) {

    printf("DEBUG: %s\n", input);

    mpc_parser_t *Ident      = mpc_new("ident");
    mpc_parser_t *TypeSpec   = mpc_new("typespec");
    mpc_parser_t *ParamP     = mpc_new("param");
    mpc_parser_t *ParamList  = mpc_new("paramlist");
    mpc_parser_t *MethodDecl = mpc_new("methoddecl");
    mpc_parser_t *AccessKw   = mpc_new("accesskw");
    mpc_parser_t *AccessBlk  = mpc_new("accessblk");
    mpc_parser_t *Iface      = mpc_new("iface");
    mpc_parser_t *ClassDecl  = mpc_new("classdecl");
    mpc_parser_t *Top        = mpc_new("top");
    mpc_parser_t *Junk       = mpc_new("junk");
    mpc_parser_t *TU         = mpc_new("tu");

    mpc_err_t *gerr = mpca_lang(MPCA_LANG_DEFAULT,
        "ident       : /[A-Za-z_][A-Za-z0-9_]*/ ;                        \n"
        "typespec    : <ident> ;                                         \n"
        "param       : <typespec> <ident> ;                              \n"
        "paramlist   : <param> (',' <param>)* ;                          \n"
        "methoddecl  : <typespec> <ident> '(' (<paramlist>)? ')' ';' ;   \n"
        "accesskw    : \"public\" | \"protected\" | \"private\" ;        \n"
        "accessblk   : <accesskw> ':' (<methoddecl> | ';')* ;            \n"
        "iface       : \"interface\" <ident> '{' (<methoddecl>)* '}' ';'? ; \n"
        "classdecl   : \"class\" <ident> '{' (<methoddecl>)* '}' ';'? ;  \n"
        "top         : <iface> | <classdecl> ;                           \n"
        "junk        : /[ \\t\\r\\n]+/ ;                                 \n"
        "tu : <junk>? <top> (<junk> <top>)* <junk>? ;                    \n"
        ,
        Ident, TypeSpec, ParamP, ParamList, MethodDecl,
        AccessKw, AccessBlk, Iface, ClassDecl, Top, Junk, TU, NULL
    );
    
    if (gerr) { 
        mpc_err_print(gerr); 
        mpc_err_delete(gerr); /* aborta e cleanup */ 
        goto cleanup;
    }

    Module M = {0};

    if (!TU || !Top || !Iface || !ClassDecl) {
        fprintf(stderr, "GRAMMAR BUILD FAILED\n");
        return M;
    }

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, TU, &r)) {
        mpc_ast_t *ast = r.output;

        mpc_ast_print(ast);

        mpc_ast_delete(ast);
        goto cleanup;

        /* Preliminars accounting */
        size_t nif = 0, ncl = 0;
        for (int i = 0; i < ast->children_num; i++) {
            if (strstr(ast->children[i]->tag, "iface"))      nif++;
            if (strstr(ast->children[i]->tag, "classdecl"))  ncl++;
        }
        M.ifaces  = (Interface*)calloc(nif, sizeof *M.ifaces);   M.nifaces  = 0;
        M.classes = (Class*)    calloc(ncl, sizeof *M.classes);  M.nclasses = 0;

        for (int i = 0; i < ast->children_num; i++) {
            mpc_ast_t *n = ast->children[i];

            /* -------- interface -------- */
            if (strstr(n->tag, "iface")) {
                Interface *iface = &M.ifaces[M.nifaces++];
                /* iface: 'interface' ident '{' (methoddecl)* '}' ';'? */
                iface->name = strdup(n->children[1]->contents);

                /* methods */
                size_t nm = 0;
                for (int k = 0; k < n->children_num; k++)
                    if (strstr(n->children[k]->tag, "methoddecl")) nm++;

                iface->methods = (Method*)calloc(nm, sizeof *iface->methods);
                iface->nmethods = 0;

                for (int k = 0; k < n->children_num; k++) if (strstr(n->children[k]->tag, "methoddecl")) {
                    mpc_ast_t *md = n->children[k];
                    Method *meth = &iface->methods[iface->nmethods++];

                    meth->ret_type = grab_typespec(md->children[0]);
                    meth->name     = strdup(md->children[1]->contents);

                    /* params */
                    meth->nparams = 0; meth->params = NULL;
                    if (md->children_num > 3 && strstr(md->children[3]->tag, "paramlist")) {
                        mpc_ast_t *pl = md->children[3];
                        size_t np = 0;
                        for (int t = 0; t < pl->children_num; t++)
                            if (strstr(pl->children[t]->tag, "param")) np++;

                        meth->params = (Param *)calloc(np, sizeof *meth->params);
                        meth->nparams = 0;

                        for (int t = 0; t < pl->children_num; t++) if (strstr(pl->children[t]->tag, "param")) {
                            mpc_ast_t *pp = pl->children[t];
                            Param *P = &meth->params[meth->nparams++];
                            P->type = grab_typespec(pp->children[0]);
                            P->name = strdup(pp->children[1]->contents);
                        }
                    }
                    meth->access = ACC_PUBLIC; /* interface => public */
                }
            }

            /* -------- class -------- */
            else if (strstr(n->tag, "classdecl")) {
                Class *cls = &M.classes[M.nclasses++];
                /* class ident (extends ident)? (implements list)? '{' ... '}' */
                cls->name    = strdup(n->children[1]->contents);
                cls->base    = NULL;
                cls->ifaces  = NULL; cls->nifaces  = 0;
                cls->fields  = NULL; cls->nfields  = 0;
                cls->methods = NULL; cls->nmethods = 0;

                Access cur = ACC_PUBLIC; /* default */

                for (int k = 0; k < n->children_num; k++) {
                    mpc_ast_t *c = n->children[k];

                    if (c->contents && strcmp(c->contents, "extends") == 0) {
                        cls->base = strdup(n->children[k+1]->contents);
                        k++; continue;
                    }

                    if (c->contents && strcmp(c->contents, "implements") == 0) {
                        /* coleta lista de idents após 'implements' */
                        size_t cnt = 0; int t = k + 1;
                        while (t < n->children_num && strstr(n->children[t]->tag, "ident")) {
                            cnt++; t++;
                            if (t < n->children_num && strcmp(n->children[t]->contents, ",") == 0) t++;
                            else break;
                        }
                        cls->ifaces = (char**)calloc(cnt, sizeof *cls->ifaces); cls->nifaces = 0;
                        t = k + 1;
                        while (t < n->children_num && strstr(n->children[t]->tag, "ident")) {
                            cls->ifaces[cls->nifaces++] = strdup(n->children[t]->contents);
                            t++;
                            if (t < n->children_num && strcmp(n->children[t]->contents, ",") == 0) t++;
                            else break;
                        }
                        k = t - 1;
                        continue;
                    }

                    if (strstr(c->tag, "accessblk")) {
                        /* accessblk : accesskw ':' ( fielddecl | methoddecl | ';')* */
                        const char *kw = c->children[0]->contents;
                        cur = strcmp(kw, "public")==0 ? ACC_PUBLIC
                            : strcmp(kw, "protected")==0 ? ACC_PROTECTED
                                                         : ACC_PRIVATE;

                        for (int t = 2; t < c->children_num; t++) {
                            mpc_ast_t *m = c->children[t];

                            if (strstr(m->tag, "fielddecl")) {
                                cls->fields = (Field*)realloc(cls->fields, (cls->nfields + 1) * sizeof *cls->fields);
                                Field *fld = &cls->fields[cls->nfields++];
                                fld->type   = grab_typespec(m->children[0]);
                                fld->name   = strdup(m->children[1]->contents);
                                fld->access = cur;
                            }
                            else if (strstr(m->tag, "methoddecl")) {
                                cls->methods = (Method*)realloc(cls->methods, (cls->nmethods + 1) * sizeof *cls->methods);
                                Method *meth = &cls->methods[cls->nmethods++];
                                meth->ret_type = grab_typespec(m->children[0]);
                                meth->name     = strdup(m->children[1]->contents);
                                meth->access   = cur;
                                meth->params   = NULL; meth->nparams = 0;

                                if (m->children_num > 3 && strstr(m->children[3]->tag, "paramlist")) {
                                    mpc_ast_t *pl = m->children[3];
                                    size_t np = 0;
                                    for (int u = 0; u < pl->children_num; u++)
                                        if (strstr(pl->children[u]->tag, "param")) np++;

                                    meth->params = (Param*)calloc(np, sizeof *meth->params);
                                    for (int u = 0, idx = 0; u < pl->children_num; u++)
                                        if (strstr(pl->children[u]->tag, "param")) {
                                            Param *P = &meth->params[idx++];
                                            mpc_ast_t *pp = pl->children[u];
                                            P->type = grab_typespec(pp->children[0]);
                                            P->name = strdup(pp->children[1]->contents);
                                        }
                                    meth->nparams = np;
                                }
                            }
                        }
                    }
                    else if (strstr(c->tag, "fielddecl")) {
                        cls->fields = (Field*)realloc(cls->fields, (cls->nfields + 1) * sizeof *cls->fields);
                        Field *fld = &cls->fields[cls->nfields++];
                        fld->type   = grab_typespec(c->children[0]);
                        fld->name   = strdup(c->children[1]->contents);
                        fld->access = cur;
                    }
                    else if (strstr(c->tag, "methoddecl")) {
                        cls->methods = (Method*)realloc(cls->methods, (cls->nmethods + 1) * sizeof *cls->methods);
                        Method *meth = &cls->methods[cls->nmethods++];
                        meth->ret_type = grab_typespec(c->children[0]);
                        meth->name     = strdup(c->children[1]->contents);
                        meth->access   = cur;
                        meth->params   = NULL; meth->nparams = 0;

                        if (c->children_num > 3 && strstr(c->children[3]->tag, "paramlist")) {
                            mpc_ast_t *pl = c->children[3];
                            size_t np = 0;
                            for (int u = 0; u < pl->children_num; u++)
                                if (strstr(pl->children[u]->tag, "param")) np++;

                            meth->params = (Param*)calloc(np, sizeof *meth->params);
                            for (int u = 0, idx = 0; u < pl->children_num; u++)
                                if (strstr(pl->children[u]->tag, "param")) {
                                    Param *P = &meth->params[idx++];
                                    mpc_ast_t *pp = pl->children[u];
                                    P->type = grab_typespec(pp->children[0]);
                                    P->name = strdup(pp->children[1]->contents);
                                }
                            meth->nparams = np;
                        }
                    }
                }
            }
        }

        mpc_ast_delete(ast);
    } else {
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
    }

cleanup:

    /* Cleaning of the created grammar */
    mpc_cleanup(12,
        Ident, TypeSpec, ParamP, ParamList, MethodDecl,
        AccessKw, AccessBlk, Iface, ClassDecl, Top, Junk, TU
    );

    return M;
}
