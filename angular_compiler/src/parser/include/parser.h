#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

typedef struct ast_file_t ast_file_t;

typedef enum {
  PARSER_FILE_UNKNOWN = 0,
  PARSER_FILE_COMPONENT,
  PARSER_FILE_TEMPLATE,
  PARSER_FILE_STYLE
} parser_file_kind_t;

typedef struct {
  parser_file_kind_t kind;
  size_t total_tokens;
  size_t significant_tokens;
  size_t identifier_count;
  size_t string_count;
  size_t template_count;
  size_t number_count;
  size_t punctuation_count;
  size_t comment_count;
  size_t component_decorator_count;
  size_t class_count;
  size_t interpolation_count;
  size_t attribute_binding_count;
  size_t block_open_count;
  size_t block_close_count;
} parse_summary_t;

int parser_summarize_file(const char *path, const char *input, size_t length, parse_summary_t *out_summary);
int parser_parse_file(const char *path, const char *input, size_t length, ast_file_t *out_file);
const char *parser_file_kind_name(parser_file_kind_t kind);
void parser_print_summary(const parse_summary_t *summary);

#endif
