#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum {
  TOKEN_IDENTIFIER = 0,
  TOKEN_NUMBER,
  TOKEN_STRING,
  TOKEN_TEMPLATE,
  TOKEN_PUNCTUATION,
  TOKEN_COMMENT,
  TOKEN_WHITESPACE,
  TOKEN_UNKNOWN,
  TOKEN_EOF
} token_kind_t;

typedef struct {
  token_kind_t kind;
  const char *start;
  size_t length;
  size_t line;
  size_t column;
} token_t;

const char *token_kind_name(token_kind_t kind);
void token_print(const token_t *token);

#endif
