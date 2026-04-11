#include "parser_internal.h"

#include <string.h>

static int token_matches_text(const token_t *token, const char *text) {
  size_t length = strlen(text);
  return token->length == length && strncmp(token->start, text, length) == 0;
}

static int token_is_noise(const token_t *token) {
  return token->kind == TOKEN_WHITESPACE || token->kind == TOKEN_COMMENT;
}

int parser_parse_template(parser_state_t *state, ast_file_t *out_file) {
  size_t index;

  ast_file_init(out_file, PARSER_FILE_TEMPLATE);

  for (index = 0; index < state->count; ++index) {
    token_t *token = &state->tokens[index];

    if (token_is_noise(token)) {
      continue;
    }

    if (token->kind == TOKEN_PUNCTUATION && token_matches_text(token, "<")) {
      if (index + 1 < state->count &&
          state->tokens[index + 1].kind == TOKEN_IDENTIFIER) {
        out_file->data.template_file.element_count += 1;
      }
    }

    if (token->kind == TOKEN_PUNCTUATION && token_matches_text(token, "{")) {
      if (index + 1 < state->count &&
          state->tokens[index + 1].kind == TOKEN_PUNCTUATION &&
          token_matches_text(&state->tokens[index + 1], "{")) {
        out_file->data.template_file.interpolation_count += 1;
      }
    }

    if (token->kind == TOKEN_PUNCTUATION && token_matches_text(token, "[")) {
      out_file->data.template_file.attribute_binding_count += 1;
    }
  }

  return 0;
}
