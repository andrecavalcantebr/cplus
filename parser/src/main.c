/*
	FILE: parser_generated.c
	DESCR: Parser MPC gerado a partir de grammar.mpc via mpca_lang (gramática embutida)
	AUTHOR: Andre Cavalcante
	DATE: August, 2025
	LICENSE: CC BY-SA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpc.h>

#include "ast.h"

static char* slurp(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  if (n < 0) {
    fclose(f);
    return NULL;
  }

  rewind(f);
  char *buf = (char*)malloc((size_t)n + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  size_t rd = fread(buf, 1, (size_t)n, f);
  fclose(f);

  buf[rd] = '\0';
  return buf;
}

static const char *GRAMMAR =
  "bom           : /\\xEF\\xBB\\xBF/ ;\n"
  "ws            : /([[:space:]]|\\/\\/[^\\n]*|\\/\\*([^*]|\\*+[^*\\/])*\\*+\\/)*/ ;\n"
  "letter        : /[A-Za-z_]/ ;\n"
  "alnum         : /[A-Za-z0-9_]/ ;\n"
  "identifier    : <letter> <alnum>* ;\n"
  "kw_interface  : \"interface\" ;\n"
  "kw_class      : \"class\" ;\n"
  "kw_public     : \"public\" ;\n"
  "kw_private    : \"private\" ;\n"
  "kw_protected  : \"protected\" ;\n"
  "type_name     : <identifier> ;\n"
  "type          : <type_name> ;\n"
  "lbrace        : \"{\" ;\n"
  "rbrace        : \"}\" ;\n"
  "lparen        : \"(\" ;\n"
  "rparen        : \")\" ;\n"
  "semi          : \";\" ;\n"
  "comma         : \",\" ;\n"
  "colon         : \":\" ;\n"
  "param         : <type> <identifier> ;\n"
  "param_list    : <param> ( <comma> <param> )* ;\n"
  "params        : <lparen> ( <param_list> )? <rparen> ;\n"
  "method_name   : <identifier> ;\n"
  "method_decl   : <type> <method_name> <params> <semi> ;\n"
  "field_decl    : <type> <identifier> <semi> ;\n"
  "member        : <method_decl> | <field_decl> ;\n"
  "access_kw     : <kw_public> | <kw_private> | <kw_protected> ;\n"
  "section       : <access_kw> <colon>\n"
  "                <ws>\n"
  "                ( <member> <ws> )*\n"
  "              ;\n"
  "interface     : <kw_interface> <ws> <identifier> <ws> <lbrace> <ws>\n"
  "                ( <method_decl> <ws> )*\n"
  "                <rbrace> <ws> <semi> ;\n"
  "class         : <kw_class> <ws> <identifier> <ws> <lbrace> <ws>\n"
  "                ( <section> <ws>\n"
  "                | <member>  <ws>    /* allow members outside sections */\n"
  "                )*\n"
  "                <rbrace> <ws> <semi> ;\n"
  "program       : <ws> ( <bom> <ws> )? ( <interface> | <class> )+ <ws> ;\n"
;

static mpc_parser_t *bom;
static mpc_parser_t *ws;
static mpc_parser_t *letter;
static mpc_parser_t *alnum;
static mpc_parser_t *identifier;
static mpc_parser_t *kw_interface;
static mpc_parser_t *kw_class;
static mpc_parser_t *kw_public;
static mpc_parser_t *kw_private;
static mpc_parser_t *kw_protected;
static mpc_parser_t *type_name;
static mpc_parser_t *type;
static mpc_parser_t *lbrace;
static mpc_parser_t *rbrace;
static mpc_parser_t *lparen;
static mpc_parser_t *rparen;
static mpc_parser_t *semi;
static mpc_parser_t *comma;
static mpc_parser_t *colon;
static mpc_parser_t *param;
static mpc_parser_t *param_list;
static mpc_parser_t *params;
static mpc_parser_t *method_name;
static mpc_parser_t *method_decl;
static mpc_parser_t *field_decl;
static mpc_parser_t *member;
static mpc_parser_t *access_kw;
static mpc_parser_t *section;
static mpc_parser_t *interface;
static mpc_parser_t *class;
static mpc_parser_t *program;

static void build_parsers(void) {
  bom = mpc_new("bom");
  ws = mpc_new("ws");
  letter = mpc_new("letter");
  alnum = mpc_new("alnum");
  identifier = mpc_new("identifier");
  kw_interface = mpc_new("kw_interface");
  kw_class = mpc_new("kw_class");
  kw_public = mpc_new("kw_public");
  kw_private = mpc_new("kw_private");
  kw_protected = mpc_new("kw_protected");
  type_name = mpc_new("type_name");
  type = mpc_new("type");
  lbrace = mpc_new("lbrace");
  rbrace = mpc_new("rbrace");
  lparen = mpc_new("lparen");
  rparen = mpc_new("rparen");
  semi = mpc_new("semi");
  comma = mpc_new("comma");
  colon = mpc_new("colon");
  param = mpc_new("param");
  param_list = mpc_new("param_list");
  params = mpc_new("params");
  method_name = mpc_new("method_name");
  method_decl = mpc_new("method_decl");
  field_decl = mpc_new("field_decl");
  member = mpc_new("member");
  access_kw = mpc_new("access_kw");
  section = mpc_new("section");
  interface = mpc_new("interface");
  class = mpc_new("class");
  program = mpc_new("program");
}

static void cleanup_parsers(void) {
  mpc_cleanup(31, bom, ws, letter, alnum, identifier, kw_interface, kw_class, kw_public, kw_private, kw_protected, type_name, type, lbrace, rbrace, lparen, rparen, semi, comma, colon, param, param_list, params, method_name, method_decl, field_decl, member, access_kw, section, interface, class, program);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Uso: %s <arquivo_para_parsear>\n", argv[0]);
    return 1;
  }

  char *input_text = slurp(argv[1]);
  if (!input_text) {
    fprintf(stderr, "Erro ao ler arquivo de entrada: %s\n", argv[1]);
    return 1;
  }

  build_parsers();

  mpc_err_t *err = mpca_lang(
    MPCA_LANG_DEFAULT,
    GRAMMAR,
    bom,
    ws,
    letter,
    alnum,
    identifier,
    kw_interface,
    kw_class,
    kw_public,
    kw_private,
    kw_protected,
    type_name,
    type,
    lbrace,
    rbrace,
    lparen,
    rparen,
    semi,
    comma,
    colon,
    param,
    param_list,
    params,
    method_name,
    method_decl,
    field_decl,
    member,
    access_kw,
    section,
    interface,
    class,
    program,
    NULL
  );

  if (err) {
    mpc_err_print(err);
    mpc_err_delete(err);
    free(input_text);
    cleanup_parsers();
    return 2;
  }

  mpc_result_t r;
  if (mpc_parse("program", input_text, program, &r)) {
    print_program(r.output);
    mpc_ast_delete(r.output);
  } else {
    mpc_err_print(r.error);
    mpc_err_delete(r.error);
    free(input_text);
    cleanup_parsers();
    return 3;
  }

  free(input_text);
  cleanup_parsers();
  return 0;
}
