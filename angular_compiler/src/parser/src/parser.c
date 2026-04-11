#include "parser.h"

#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "log.h"
#include "parser_internal.h"
#include "token.h"
#include "tokenizer.h"

static int path_has_suffix(const char *path, const char *suffix) {
  size_t path_length = strlen(path);
  size_t suffix_length = strlen(suffix);

  if (path_length < suffix_length) {
    return 0;
  }

  return strcmp(path + path_length - suffix_length, suffix) == 0;
}

static parser_file_kind_t parser_detect_kind(const char *path) {
  if (path_has_suffix(path, ".ng")) {
    return PARSER_FILE_COMPONENT;
  }
  if (path_has_suffix(path, ".html")) {
    return PARSER_FILE_TEMPLATE;
  }
  if (path_has_suffix(path, ".css")) {
    return PARSER_FILE_STYLE;
  }
  return PARSER_FILE_UNKNOWN;
}

static int token_is_significant(const token_t *token) {
  return token->kind != TOKEN_WHITESPACE && token->kind != TOKEN_COMMENT && token->kind != TOKEN_EOF;
}

static int parser_collect_tokens(const char *input,
                                 size_t length,
                                 token_t **out_tokens,
                                 size_t *out_count) {
  tokenizer_t tokenizer;
  token_t *tokens = NULL;
  size_t capacity = 0;
  size_t count = 0;

  tokenizer_init(&tokenizer, input, length);

  for (;;) {
    token_t token = tokenizer_next(&tokenizer);
    if (count == capacity) {
      size_t next_capacity = capacity == 0 ? 256 : capacity * 2;
      token_t *next_tokens = (token_t *)realloc(tokens, next_capacity * sizeof(token_t));
      if (next_tokens == NULL) {
        free(tokens);
        return 1;
      }
      tokens = next_tokens;
      capacity = next_capacity;
    }

    tokens[count++] = token;
    if (token.kind == TOKEN_EOF) {
      break;
    }
  }

  *out_tokens = tokens;
  *out_count = count;
  return 0;
}

static void parser_fill_summary_from_tokens(const token_t *tokens, size_t count, parse_summary_t *out_summary) {
  size_t index;

  for (index = 0; index < count; ++index) {
    const token_t *token = &tokens[index];
    out_summary->total_tokens += 1;

    if (token_is_significant(token)) {
      out_summary->significant_tokens += 1;
    }

    switch (token->kind) {
      case TOKEN_IDENTIFIER:
        out_summary->identifier_count += 1;
        break;
      case TOKEN_STRING:
        out_summary->string_count += 1;
        break;
      case TOKEN_TEMPLATE:
        out_summary->template_count += 1;
        break;
      case TOKEN_NUMBER:
        out_summary->number_count += 1;
        break;
      case TOKEN_PUNCTUATION:
        out_summary->punctuation_count += 1;
        break;
      case TOKEN_COMMENT:
        out_summary->comment_count += 1;
        break;
      case TOKEN_WHITESPACE:
      case TOKEN_UNKNOWN:
      case TOKEN_EOF:
        break;
    }
  }
}

static void parser_fill_summary_from_ast(const ast_file_t *ast, parse_summary_t *out_summary) {
  switch (ast->kind) {
    case PARSER_FILE_COMPONENT:
      out_summary->component_decorator_count = ast->data.component.has_component_decorator ? 1u : 0u;
      out_summary->class_count = ast->data.component.class_name[0] != '\0' ? 1u : 0u;
      break;
    case PARSER_FILE_TEMPLATE:
      out_summary->interpolation_count = ast->data.template_file.interpolation_count;
      out_summary->attribute_binding_count = ast->data.template_file.attribute_binding_count;
      out_summary->block_open_count = ast->data.template_file.interpolation_count * 2;
      out_summary->block_close_count = ast->data.template_file.interpolation_count * 2;
      break;
    case PARSER_FILE_STYLE:
      out_summary->block_open_count = ast->data.style.rule_count;
      out_summary->block_close_count = ast->data.style.rule_count;
      break;
    case PARSER_FILE_UNKNOWN:
      break;
  }
}

int parser_parse_file(const char *path, const char *input, size_t length, ast_file_t *out_file) {
  parser_state_t state;
  token_t *tokens = NULL;
  size_t count = 0;
  int result = 0;

  if (parser_collect_tokens(input, length, &tokens, &count) != 0) {
    LOG_TRACE("parser_parse_file token collection failed path=%s length=%zu\n", path, length);
    return 1;
  }

  LOG_TRACE("parser_parse_file start path=%s length=%zu token_count=%zu kind=%s\n",
            path,
            length,
            count,
            parser_file_kind_name(parser_detect_kind(path)));

  state.path = path;
  state.tokens = tokens;
  state.count = count;
  state.index = 0;

  switch (parser_detect_kind(path)) {
    case PARSER_FILE_COMPONENT:
      result = parser_parse_component(&state, out_file);
      break;
    case PARSER_FILE_TEMPLATE:
      result = parser_parse_template(&state, out_file);
      break;
    case PARSER_FILE_STYLE:
      result = parser_parse_style(&state, out_file);
      break;
    case PARSER_FILE_UNKNOWN:
      ast_file_init(out_file, PARSER_FILE_UNKNOWN);
      result = 0;
      break;
  }

  LOG_TRACE("parser_parse_file finish path=%s result=%d ast_kind=%s\n",
            path,
            result,
            parser_file_kind_name(out_file->kind));
  free(tokens);
  return result;
}

int parser_summarize_file(const char *path, const char *input, size_t length, parse_summary_t *out_summary) {
  ast_file_t ast;
  token_t *tokens = NULL;
  size_t count = 0;
  int result;

  memset(out_summary, 0, sizeof(*out_summary));
  out_summary->kind = parser_detect_kind(path);
  LOG_TRACE("parser_summarize_file start path=%s length=%zu kind=%s\n",
            path,
            length,
            parser_file_kind_name(out_summary->kind));

  if (parser_collect_tokens(input, length, &tokens, &count) != 0) {
    LOG_TRACE("parser_summarize_file token collection failed path=%s\n", path);
    return 1;
  }

  parser_fill_summary_from_tokens(tokens, count, out_summary);
  LOG_TRACE("parser_summarize_file token summary path=%s total=%zu significant=%zu\n",
            path,
            out_summary->total_tokens,
            out_summary->significant_tokens);
  free(tokens);

  result = parser_parse_file(path, input, length, &ast);
  if (result != 0) {
    LOG_TRACE("parser_summarize_file parse failed path=%s result=%d\n", path, result);
    return result;
  }

  parser_fill_summary_from_ast(&ast, out_summary);
  LOG_TRACE("parser_summarize_file finish path=%s decorators=%zu classes=%zu interpolations=%zu attr_bindings=%zu\n",
            path,
            out_summary->component_decorator_count,
            out_summary->class_count,
            out_summary->interpolation_count,
            out_summary->attribute_binding_count);
  return 0;
}

const char *parser_file_kind_name(parser_file_kind_t kind) {
  switch (kind) {
    case PARSER_FILE_COMPONENT:
      return "component";
    case PARSER_FILE_TEMPLATE:
      return "template";
    case PARSER_FILE_STYLE:
      return "style";
    case PARSER_FILE_UNKNOWN:
      return "unknown";
  }

  return "invalid";
}

void parser_print_summary(const parse_summary_t *summary) {
  log_printf("SUMMARY kind=%s total=%zu significant=%zu identifiers=%zu strings=%zu templates=%zu numbers=%zu punctuation=%zu comments=%zu decorators=%zu classes=%zu interpolations=%zu attr_bindings=%zu braces=%zu/%zu\n",
             parser_file_kind_name(summary->kind),
             summary->total_tokens,
             summary->significant_tokens,
             summary->identifier_count,
             summary->string_count,
             summary->template_count,
             summary->number_count,
             summary->punctuation_count,
             summary->comment_count,
             summary->component_decorator_count,
             summary->class_count,
             summary->interpolation_count,
             summary->attribute_binding_count,
             summary->block_open_count,
             summary->block_close_count);
}
