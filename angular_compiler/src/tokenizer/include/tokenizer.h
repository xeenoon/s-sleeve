#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "token.h"

typedef struct {
  const char *input;
  size_t length;
  size_t offset;
  size_t line;
  size_t column;
} tokenizer_t;

void tokenizer_init(tokenizer_t *tokenizer, const char *input, size_t length);
token_t tokenizer_next(tokenizer_t *tokenizer);

#endif
