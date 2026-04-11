#ifndef AST_H
#define AST_H

#include <stddef.h>

#include "parser.h"

#define AST_MAX_MEMBERS 64
#define AST_MAX_METHOD_PARAMS 8
#define AST_MAX_METHOD_BODY 4096

typedef enum {
  AST_MEMBER_FIELD = 0,
  AST_MEMBER_METHOD = 1
} ast_member_kind_t;

typedef struct {
  char name[128];
  ast_member_kind_t kind;
  int uses_external_fetch;
  char params[AST_MAX_METHOD_PARAMS][64];
  size_t param_count;
  char body[AST_MAX_METHOD_BODY];
} ast_member_t;

typedef struct {
  int has_component_decorator;
  char class_name[128];
  ast_member_t members[AST_MAX_MEMBERS];
  size_t member_count;
} ast_component_file_t;

typedef struct {
  size_t element_count;
  size_t interpolation_count;
  size_t attribute_binding_count;
} ast_template_file_t;

typedef struct {
  size_t rule_count;
  size_t block_depth;
  size_t max_block_depth;
  int has_balanced_blocks;
} ast_style_file_t;

struct ast_file_t {
  parser_file_kind_t kind;
  union {
    ast_component_file_t component;
    ast_template_file_t template_file;
    ast_style_file_t style;
  } data;
};

typedef struct ast_file_t ast_file_t;

void ast_file_init(ast_file_t *file, parser_file_kind_t kind);
void ast_file_print(const ast_file_t *file);

#endif
