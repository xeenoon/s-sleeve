#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include "ast.h"
#include "token.h"

typedef struct {
  const char *path;
  token_t *tokens;
  size_t count;
  size_t index;
} parser_state_t;

int parser_parse_component(parser_state_t *state, ast_file_t *out_file);
int parser_parse_template(parser_state_t *state, ast_file_t *out_file);
int parser_parse_style(parser_state_t *state, ast_file_t *out_file);

#endif
