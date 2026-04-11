#include "token.h"

#include <stdio.h>

#include "log.h"

static void print_escaped_slice(const char *text, size_t length) {
  size_t index;

  log_putc('"');
  for (index = 0; index < length; ++index) {
    unsigned char ch = (unsigned char)text[index];
    if (ch == '\n') {
      log_puts("\\n");
    } else if (ch == '\r') {
      log_puts("\\r");
    } else if (ch == '\t') {
      log_puts("\\t");
    } else if (ch == '\\') {
      log_puts("\\\\");
    } else if (ch == '"') {
      log_puts("\\\"");
    } else if (ch < 32 || ch > 126) {
      log_printf("\\x%02X", ch);
    } else {
      log_putc((int)ch);
    }
  }
  log_putc('"');
}

const char *token_kind_name(token_kind_t kind) {
  switch (kind) {
    case TOKEN_IDENTIFIER:
      return "IDENTIFIER";
    case TOKEN_NUMBER:
      return "NUMBER";
    case TOKEN_STRING:
      return "STRING";
    case TOKEN_TEMPLATE:
      return "TEMPLATE";
    case TOKEN_PUNCTUATION:
      return "PUNCT";
    case TOKEN_COMMENT:
      return "COMMENT";
    case TOKEN_WHITESPACE:
      return "WHITESPACE";
    case TOKEN_UNKNOWN:
      return "UNKNOWN";
    case TOKEN_EOF:
      return "EOF";
  }

  return "INVALID";
}

void token_print(const token_t *token) {
  log_printf("%zu:%zu %-10s ", token->line, token->column, token_kind_name(token->kind));
  print_escaped_slice(token->start, token->length);
  log_putc('\n');
}
