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
  "identifier : /[A-Za-z_][A-Za-z0-9_]*/ ;\n"
  "number     : /0|[1-9][0-9]*/ ;\n"
  "string     : /\"(\\\\.|[^\"\\\\])*\"/ ;\n"
  "charlit    : /'([^'\\\\]|\\\\.)*'/ ;\n"
  "\n"
  "ws : /([[:space:]]|\\/\\/[^\\n]*|\\/\\*([^*]|\\*+[^*\\/])*\\*+\\/)*/ ;\n"
  "\n"
  "qualifier : \"const\" | \"volatile\" | \"restrict\" | \"_Atomic\"\n"
  "          | \"signed\" | \"unsigned\" | \"short\" | \"long\"\n"
  "          | \"public\" | \"protected\" | \"private\" ;\n"
  "\n"
  "pointer : \"*\" <pointer>? ;\n"
  "\n"
  "taggedtype_use : (\"struct\" | \"class\" | \"interface\" | \"union\" | \"enum\") <identifier> ;\n"
  "\n"
  "type : <qualifier>* ( <taggedtype_use> | <identifier> ) <pointer>* ;\n"
  "\n"
  "array_suffix : \"[\" <number>? \"]\" ;\n"
  "declarator   : <pointer>* <identifier> <array_suffix>* ;\n"
  "\n"
  "constant        : <number> | <charlit> | <string> ;\n"
  "init_declarator : <declarator> (\"=\" <constant>)? ;\n"
  "var_decl        : <type> <init_declarator> (\",\" <init_declarator>)* \";\" ;\n"
  "\n"
  "param_declarator : <declarator> ;\n"
  "param            : <type> <param_declarator>? ;\n"
  "param_list       : <param> (\",\" <param>)* | \"void\" ;\n"
  "func_prototype   : <type> <identifier> \"(\" <param_list>? \")\" \";\" ;\n"
  "\n"
  "function_def : <type> <identifier> \"(\" <param_list>? \")\" <compound_stmt> ;\n"
  "\n"
  "member_decl : <var_decl> | <func_prototype> ;\n"
  "\n"
  "struct_def\n"
  "  : (\"struct\" | \"class\" | \"interface\") <identifier>? \"{\" <member_decl>* \"}\" \";\" ;\n"
  "\n"
  "primary        : <number> | <string> | <charlit> | <identifier> | \"(\" <expr> \")\" ;\n"
  "call           : <identifier> \"(\" (<expr> (\",\" <expr>)*)? \")\" ;\n"
  "unary          : (\"+\" | \"-\" | \"!\" | \"&\" | \"*\") <unary> | <call> | <primary> ;\n"
  "mul            : <unary> ((\"*\" | \"/\") <unary>)* ;\n"
  "add            : <mul>   ((\"+\" | \"-\") <mul>)* ;\n"
  "rel            : <add>   ((\"<\" | \">\" | \"<=\" | \">=\") <add>)* ;\n"
  "eq             : <rel>   ((\"==\" | \"!=\") <rel>)* ;\n"
  "land           : <eq>    (\"&&\" <eq>)* ;\n"
  "lor            : <land>  (\"||\" <land>)* ;\n"
  "assign         : <lor>   ((\"=\" | \"+=\" | \"-=\" | \"*=\" | \"/=\") <assign>)? ;\n"
  "expr           : <assign> ;\n"
  "\n"
  "expr_stmt      : <expr>? \";\" ;\n"
  "return_stmt    : \"return\" <expr>? \";\" ;\n"
  "if_stmt        : \"if\" \"(\" <expr> \")\" <stmt> (\"else\" <stmt>)? ;\n"
  "while_stmt     : \"while\" \"(\" <expr> \")\" <stmt> ;\n"
  "for_stmt       : \"for\" \"(\" <expr>? \";\" <expr>? \";\" <expr>? \")\" <stmt> ;\n"
  "compound_stmt  : \"{\" <stmt>* \"}\" ;\n"
  "\n"
  "stmt\n"
  "  : <var_decl>\n"
  "  | <return_stmt>\n"
  "  | <if_stmt>\n"
  "  | <while_stmt>\n"
  "  | <for_stmt>\n"
  "  | <compound_stmt>\n"
  "  | <expr_stmt>\n"
  "  ;\n"
  "\n"
  "external_decl\n"
  "  : <function_def>\n"
  "  | <struct_def>\n"
  "  | <func_prototype>\n"
  "  | <var_decl>\n"
  "  ;\n"
  "\n"
  "translation_unit : <external_decl>* ;\n"
  "\n"
  "c_grammar : <translation_unit> ;\n"
;

static mpc_parser_t *identifier;
static mpc_parser_t *number;
static mpc_parser_t *string;
static mpc_parser_t *charlit;
static mpc_parser_t *ws;
static mpc_parser_t *space;
static mpc_parser_t *qualifier;
static mpc_parser_t *pointer;
static mpc_parser_t *taggedtype_use;
static mpc_parser_t *type;
static mpc_parser_t *array_suffix;
static mpc_parser_t *declarator;
static mpc_parser_t *constant;
static mpc_parser_t *init_declarator;
static mpc_parser_t *var_decl;
static mpc_parser_t *param_declarator;
static mpc_parser_t *param;
static mpc_parser_t *param_list;
static mpc_parser_t *func_prototype;
static mpc_parser_t *function_def;
static mpc_parser_t *member_decl;
static mpc_parser_t *struct_def;
static mpc_parser_t *primary;
static mpc_parser_t *call;
static mpc_parser_t *unary;
static mpc_parser_t *mul;
static mpc_parser_t *add;
static mpc_parser_t *rel;
static mpc_parser_t *eq;
static mpc_parser_t *land;
static mpc_parser_t *lor;
static mpc_parser_t *assign;
static mpc_parser_t *expr;
static mpc_parser_t *expr_stmt;
static mpc_parser_t *return_stmt;
static mpc_parser_t *if_stmt;
static mpc_parser_t *while_stmt;
static mpc_parser_t *for_stmt;
static mpc_parser_t *compound_stmt;
static mpc_parser_t *stmt;
static mpc_parser_t *external_decl;
static mpc_parser_t *translation_unit;
static mpc_parser_t *c_grammar;

static void build_parsers(void) {
  identifier = mpc_new("identifier");
  number = mpc_new("number");
  string = mpc_new("string");
  charlit = mpc_new("charlit");
  ws = mpc_new("ws");
  space = mpc_new("space");
  qualifier = mpc_new("qualifier");
  pointer = mpc_new("pointer");
  taggedtype_use = mpc_new("taggedtype_use");
  type = mpc_new("type");
  array_suffix = mpc_new("array_suffix");
  declarator = mpc_new("declarator");
  constant = mpc_new("constant");
  init_declarator = mpc_new("init_declarator");
  var_decl = mpc_new("var_decl");
  param_declarator = mpc_new("param_declarator");
  param = mpc_new("param");
  param_list = mpc_new("param_list");
  func_prototype = mpc_new("func_prototype");
  function_def = mpc_new("function_def");
  member_decl = mpc_new("member_decl");
  struct_def = mpc_new("struct_def");
  primary = mpc_new("primary");
  call = mpc_new("call");
  unary = mpc_new("unary");
  mul = mpc_new("mul");
  add = mpc_new("add");
  rel = mpc_new("rel");
  eq = mpc_new("eq");
  land = mpc_new("land");
  lor = mpc_new("lor");
  assign = mpc_new("assign");
  expr = mpc_new("expr");
  expr_stmt = mpc_new("expr_stmt");
  return_stmt = mpc_new("return_stmt");
  if_stmt = mpc_new("if_stmt");
  while_stmt = mpc_new("while_stmt");
  for_stmt = mpc_new("for_stmt");
  compound_stmt = mpc_new("compound_stmt");
  stmt = mpc_new("stmt");
  external_decl = mpc_new("external_decl");
  translation_unit = mpc_new("translation_unit");
  c_grammar = mpc_new("c_grammar");
}

static void cleanup_parsers(void) {
  mpc_cleanup(43, identifier, number, string, charlit, ws, space, qualifier, pointer, taggedtype_use, type, array_suffix, declarator, constant, init_declarator, var_decl, param_declarator, param, param_list, func_prototype, function_def, member_decl, struct_def, primary, call, unary, mul, add, rel, eq, land, lor, assign, expr, expr_stmt, return_stmt, if_stmt, while_stmt, for_stmt, compound_stmt, stmt, external_decl, translation_unit, c_grammar);
}

int main(int argc, char **argv) {
  if (strcmp(argv[1], "-G") == 0) {
       puts(" ===== GRAMMAR BEGIN ===== ");
       puts(GRAMMAR);
       puts(" ===== GRAMMAR END ===== ");
       return 0;
  }
  
  if (argc < 3) {
    fprintf(stderr,
      "Uso:\n"
      "  %s -G \n"
      "  %s -f <arquivo>\n"
      "  %s -x \"<codigo C>\"\n"
      "Ex.:\n"
      "  %s -G \n"
      "  %s -f tests/01_class_basic.cplus.h\n"
      "  %s -x \"int f(void);\"\n",
      argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);
    return 1;
  }

  const char *mode = argv[1];
  char *input_text = NULL;

  if (strcmp(mode, "-f") == 0) {
    input_text = slurp(argv[2]);
    if (!input_text) {
      fprintf(stderr, "Erro ao ler arquivo: %s\n", argv[2]);
      return 1;
    }
  } else if (strcmp(mode, "-x") == 0) {
    /* Código vem como um único argumento já entre aspas na shell */
    input_text = strdup(argv[2]);
    if (!input_text) {
      fprintf(stderr, "Sem memória para -x\n");
      return 1;
    }
  } else {
    fprintf(stderr, "Parâmetro inválido: %s\n", mode);
    return 1;
  }

  build_parsers();

  mpc_err_t *err = mpca_lang(
    MPCA_LANG_DEFAULT,
    GRAMMAR,
    identifier,
    number,
    string,
    charlit,
    ws,
    space,
    qualifier,
    pointer,
    taggedtype_use,
    type,
    array_suffix,
    declarator,
    constant,
    init_declarator,
    var_decl,
    param_declarator,
    param,
    param_list,
    func_prototype,
    function_def,
    member_decl,
    struct_def,
    primary,
    call,
    unary,
    mul,
    add,
    rel,
    eq,
    land,
    lor,
    assign,
    expr,
    expr_stmt,
    return_stmt,
    if_stmt,
    while_stmt,
    for_stmt,
    compound_stmt,
    stmt,
    external_decl,
    translation_unit,
    c_grammar,
    NULL
  );

  if (err) {
    mpc_err_print(err);
    mpc_err_delete(err);
    free(input_text);
    cleanup_parsers();
    return 2;
  }

  mpc_result_t r; int ok = 0;

  if (strcmp(mode, "-f") == 0) {
    ok = mpc_parse("<file>", input_text, translation_unit, &r);
  } else { /* -x */
    /* Tenta TU primeiro; depois formas úteis para snippet curto */
    ok = mpc_parse("<snippet-tu>",   input_text, translation_unit, &r);
    if (!ok) ok = mpc_parse("<snippet-ext>",  input_text, external_decl, &r);
    if (!ok) ok = mpc_parse("<snippet-var>",  input_text, var_decl,      &r);
    if (!ok) ok = mpc_parse("<snippet-stmt>", input_text, stmt,          &r);
    if (!ok) ok = mpc_parse("<snippet-expr>", input_text, expr,          &r);
  }

  if (ok) {
    mpc_ast_t *ast = (mpc_ast_t*)r.output;    fprintf(stderr, "[debug] raiz: %s\n", ast ? ast->tag : "<null>");
    puts("===== RAW AST =====");
    mpc_ast_print(ast);
    puts("===== /RAW AST =====");
    /* Pretty-printer específico (pode imprimir NULL se esperar 'program') */
    print_program(ast);
    mpc_ast_delete(ast);
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
