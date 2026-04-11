#include "parser_internal.h"

#include <string.h>

static int token_matches_text(const token_t *token, const char *text) {
  size_t length = strlen(text);
  return token->length == length && strncmp(token->start, text, length) == 0;
}

int parser_parse_style(parser_state_t *state, ast_file_t *out_file) {
  size_t index;

  ast_file_init(out_file, PARSER_FILE_STYLE);

  for (index = 0; index < state->count; ++index) {
    token_t *token = &state->tokens[index];

    if (token->kind != TOKEN_PUNCTUATION) {
      continue;
    }

    if (token_matches_text(token, "{")) {
      out_file->data.style.rule_count += 1;
      out_file->data.style.block_depth += 1;
      if (out_file->data.style.block_depth > out_file->data.style.max_block_depth) {
        out_file->data.style.max_block_depth = out_file->data.style.block_depth;
      }
    } else if (token_matches_text(token, "}")) {
      if (out_file->data.style.block_depth > 0) {
        out_file->data.style.block_depth -= 1;
      }
    }
  }

  out_file->data.style.has_balanced_blocks = out_file->data.style.block_depth == 0;
  return 0;
}
