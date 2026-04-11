#include "tokenizer.h"

#include <ctype.h>

static int tokenizer_is_at_end(const tokenizer_t *tokenizer) {
  return tokenizer->offset >= tokenizer->length;
}

static char tokenizer_peek(const tokenizer_t *tokenizer) {
  if (tokenizer_is_at_end(tokenizer)) {
    return '\0';
  }
  return tokenizer->input[tokenizer->offset];
}

static char tokenizer_peek_next(const tokenizer_t *tokenizer) {
  if (tokenizer->offset + 1 >= tokenizer->length) {
    return '\0';
  }
  return tokenizer->input[tokenizer->offset + 1];
}

static char tokenizer_advance(tokenizer_t *tokenizer) {
  char ch = tokenizer_peek(tokenizer);

  if (ch == '\0') {
    return ch;
  }

  tokenizer->offset += 1;
  if (ch == '\n') {
    tokenizer->line += 1;
    tokenizer->column = 1;
  } else {
    tokenizer->column += 1;
  }

  return ch;
}

static token_t tokenizer_make_token(tokenizer_t *tokenizer,
                                    token_kind_t kind,
                                    size_t start_offset,
                                    size_t start_line,
                                    size_t start_column) {
  token_t token;
  token.kind = kind;
  token.start = tokenizer->input + start_offset;
  token.length = tokenizer->offset - start_offset;
  token.line = start_line;
  token.column = start_column;
  return token;
}

static token_t tokenizer_consume_whitespace(tokenizer_t *tokenizer,
                                            size_t start_offset,
                                            size_t start_line,
                                            size_t start_column) {
  while (isspace((unsigned char)tokenizer_peek(tokenizer)) != 0) {
    tokenizer_advance(tokenizer);
  }

  return tokenizer_make_token(tokenizer, TOKEN_WHITESPACE, start_offset, start_line, start_column);
}

static token_t tokenizer_consume_identifier(tokenizer_t *tokenizer,
                                            size_t start_offset,
                                            size_t start_line,
                                            size_t start_column) {
  while (isalnum((unsigned char)tokenizer_peek(tokenizer)) != 0 ||
         tokenizer_peek(tokenizer) == '_' ||
         tokenizer_peek(tokenizer) == '$') {
    tokenizer_advance(tokenizer);
  }

  return tokenizer_make_token(tokenizer, TOKEN_IDENTIFIER, start_offset, start_line, start_column);
}

static token_t tokenizer_consume_number(tokenizer_t *tokenizer,
                                        size_t start_offset,
                                        size_t start_line,
                                        size_t start_column) {
  while (isdigit((unsigned char)tokenizer_peek(tokenizer)) != 0 ||
         tokenizer_peek(tokenizer) == '.') {
    tokenizer_advance(tokenizer);
  }

  return tokenizer_make_token(tokenizer, TOKEN_NUMBER, start_offset, start_line, start_column);
}

static token_t tokenizer_consume_comment(tokenizer_t *tokenizer,
                                         size_t start_offset,
                                         size_t start_line,
                                         size_t start_column) {
  if (tokenizer_peek(tokenizer) == '/' && tokenizer_peek_next(tokenizer) == '/') {
    tokenizer_advance(tokenizer);
    tokenizer_advance(tokenizer);
    while (!tokenizer_is_at_end(tokenizer) && tokenizer_peek(tokenizer) != '\n') {
      tokenizer_advance(tokenizer);
    }
  } else {
    tokenizer_advance(tokenizer);
    tokenizer_advance(tokenizer);
    while (!tokenizer_is_at_end(tokenizer)) {
      if (tokenizer_peek(tokenizer) == '*' && tokenizer_peek_next(tokenizer) == '/') {
        tokenizer_advance(tokenizer);
        tokenizer_advance(tokenizer);
        break;
      }
      tokenizer_advance(tokenizer);
    }
  }

  return tokenizer_make_token(tokenizer, TOKEN_COMMENT, start_offset, start_line, start_column);
}

static token_t tokenizer_consume_quoted(tokenizer_t *tokenizer,
                                        char delimiter,
                                        token_kind_t kind,
                                        size_t start_offset,
                                        size_t start_line,
                                        size_t start_column) {
  tokenizer_advance(tokenizer);

  while (!tokenizer_is_at_end(tokenizer)) {
    char ch = tokenizer_advance(tokenizer);
    if (ch == '\\') {
      if (!tokenizer_is_at_end(tokenizer)) {
        tokenizer_advance(tokenizer);
      }
      continue;
    }
    if (ch == delimiter) {
      break;
    }
  }

  return tokenizer_make_token(tokenizer, kind, start_offset, start_line, start_column);
}

void tokenizer_init(tokenizer_t *tokenizer, const char *input, size_t length) {
  tokenizer->input = input;
  tokenizer->length = length;
  tokenizer->offset = 0;
  tokenizer->line = 1;
  tokenizer->column = 1;
}

token_t tokenizer_next(tokenizer_t *tokenizer) {
  size_t start_offset = tokenizer->offset;
  size_t start_line = tokenizer->line;
  size_t start_column = tokenizer->column;
  char ch = tokenizer_peek(tokenizer);

  if (ch == '\0') {
    return tokenizer_make_token(tokenizer, TOKEN_EOF, start_offset, start_line, start_column);
  }

  if (isspace((unsigned char)ch) != 0) {
    return tokenizer_consume_whitespace(tokenizer, start_offset, start_line, start_column);
  }

  if (isalpha((unsigned char)ch) != 0 || ch == '_' || ch == '$') {
    return tokenizer_consume_identifier(tokenizer, start_offset, start_line, start_column);
  }

  if (isdigit((unsigned char)ch) != 0) {
    return tokenizer_consume_number(tokenizer, start_offset, start_line, start_column);
  }

  if (ch == '/' && (tokenizer_peek_next(tokenizer) == '/' || tokenizer_peek_next(tokenizer) == '*')) {
    return tokenizer_consume_comment(tokenizer, start_offset, start_line, start_column);
  }

  if (ch == '\'') {
    return tokenizer_consume_quoted(tokenizer, '\'', TOKEN_STRING, start_offset, start_line, start_column);
  }

  if (ch == '"') {
    return tokenizer_consume_quoted(tokenizer, '"', TOKEN_STRING, start_offset, start_line, start_column);
  }

  if (ch == '`') {
    return tokenizer_consume_quoted(tokenizer, '`', TOKEN_TEMPLATE, start_offset, start_line, start_column);
  }

  tokenizer_advance(tokenizer);
  return tokenizer_make_token(tokenizer, TOKEN_PUNCTUATION, start_offset, start_line, start_column);
}
