#include "js_codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "tokenizer.h"

#define JS_MAX_TOKENS 512
#define JS_MAX_PARAMS 8
#define JS_MAX_STATEMENTS 32
#define JS_MAX_ARGS 8
#define JS_MAX_SYMBOLS 64

typedef struct js_expr_t js_expr_t;

typedef enum {
  JS_EXPR_NUMBER = 0,
  JS_EXPR_IDENTIFIER,
  JS_EXPR_MEMBER,
  JS_EXPR_CALL,
  JS_EXPR_BINARY
} js_expr_kind_t;

struct js_expr_t {
  js_expr_kind_t kind;
  char text[128];
  char member_name[64];
  char object_name[64];
  double number_value;
  char op[4];
  js_expr_t *left;
  js_expr_t *right;
  js_expr_t *args[JS_MAX_ARGS];
  size_t arg_count;
  js_type_t inferred_type;
};

typedef enum {
  JS_STMT_CONST = 0,
  JS_STMT_ASSIGN,
  JS_STMT_RETURN
} js_stmt_kind_t;

typedef struct {
  js_stmt_kind_t kind;
  char name[64];
  char object_name[64];
  char member_name[64];
  js_expr_t *expr;
} js_stmt_t;

typedef struct {
  token_t tokens[JS_MAX_TOKENS];
  size_t count;
  size_t index;
  js_expr_t expr_pool[128];
  size_t expr_count;
  js_stmt_t statements[JS_MAX_STATEMENTS];
  size_t statement_count;
} js_parser_t;

typedef struct {
  char name[64];
  js_type_t type;
} js_symbol_t;

typedef struct {
  js_symbol_t symbols[JS_MAX_SYMBOLS];
  size_t count;
} js_symbol_table_t;

static void js_copy_text(char *buffer, size_t buffer_size, const char *text) {
  if (buffer_size == 0) {
    return;
  }
  snprintf(buffer, buffer_size, "%.*s", (int)(buffer_size - 1), text);
}

static const token_t *js_current(const js_parser_t *parser) {
  if (parser->index >= parser->count) {
    return &parser->tokens[parser->count - 1];
  }
  return &parser->tokens[parser->index];
}

static int js_token_matches(const token_t *token, const char *text) {
  size_t length = strlen(text);
  return token->length == length && strncmp(token->start, text, length) == 0;
}

static int js_is_significant(const token_t *token) {
  return token->kind != TOKEN_WHITESPACE && token->kind != TOKEN_COMMENT && token->kind != TOKEN_EOF;
}

static void js_advance(js_parser_t *parser) {
  if (parser->index < parser->count) {
    parser->index += 1;
  }
}

static void js_skip_noise(js_parser_t *parser) {
  while (parser->index < parser->count && !js_is_significant(js_current(parser))) {
    parser->index += 1;
  }
}

static int js_accept_punct(js_parser_t *parser, const char *text) {
  js_skip_noise(parser);
  if (js_current(parser)->kind == TOKEN_PUNCTUATION && js_token_matches(js_current(parser), text)) {
    js_advance(parser);
    return 1;
  }
  return 0;
}

static int js_accept_identifier(js_parser_t *parser, char *buffer, size_t buffer_size) {
  js_skip_noise(parser);
  if (js_current(parser)->kind != TOKEN_IDENTIFIER) {
    return 0;
  }
  {
    size_t length = js_current(parser)->length;
    if (length >= buffer_size) {
      length = buffer_size - 1;
    }
    memcpy(buffer, js_current(parser)->start, length);
    buffer[length] = '\0';
  }
  js_advance(parser);
  return 1;
}

static js_expr_t *js_new_expr(js_parser_t *parser) {
  if (parser->expr_count >= sizeof(parser->expr_pool) / sizeof(parser->expr_pool[0])) {
    return NULL;
  }
  memset(&parser->expr_pool[parser->expr_count], 0, sizeof(parser->expr_pool[parser->expr_count]));
  parser->expr_pool[parser->expr_count].inferred_type = JS_TYPE_UNKNOWN;
  parser->expr_count += 1;
  return &parser->expr_pool[parser->expr_count - 1];
}

static js_expr_t *js_parse_expression(js_parser_t *parser);

static js_expr_t *js_parse_primary(js_parser_t *parser) {
  js_expr_t *expr;
  char ident[64];

  js_skip_noise(parser);
  if (js_accept_punct(parser, "(")) {
    expr = js_parse_expression(parser);
    js_accept_punct(parser, ")");
    return expr;
  }

  if (js_current(parser)->kind == TOKEN_NUMBER) {
    expr = js_new_expr(parser);
    if (expr == NULL) {
      return NULL;
    }
    expr->kind = JS_EXPR_NUMBER;
    {
      size_t length = js_current(parser)->length;
      if (length >= sizeof(expr->text)) {
        length = sizeof(expr->text) - 1;
      }
      memcpy(expr->text, js_current(parser)->start, length);
      expr->text[length] = '\0';
      expr->number_value = atof(expr->text);
      expr->inferred_type = strchr(expr->text, '.') != NULL ? JS_TYPE_DOUBLE : JS_TYPE_INT;
    }
    js_advance(parser);
    return expr;
  }

  if (!js_accept_identifier(parser, ident, sizeof(ident))) {
    return NULL;
  }

  expr = js_new_expr(parser);
  if (expr == NULL) {
    return NULL;
  }
  expr->kind = JS_EXPR_IDENTIFIER;
  snprintf(expr->text, sizeof(expr->text), "%s", ident);

  while (1) {
    js_skip_noise(parser);
    if (js_accept_punct(parser, ".")) {
      char member[64];
      js_expr_t *member_expr = js_new_expr(parser);
      if (member_expr == NULL || !js_accept_identifier(parser, member, sizeof(member))) {
        return NULL;
      }
      member_expr->kind = JS_EXPR_MEMBER;
      js_copy_text(member_expr->object_name, sizeof(member_expr->object_name), expr->text);
      js_copy_text(member_expr->member_name, sizeof(member_expr->member_name), member);
      snprintf(member_expr->text,
               sizeof(member_expr->text),
               "%.*s.%.*s",
               (int)(sizeof(member_expr->object_name) - 1),
               member_expr->object_name,
               (int)(sizeof(member_expr->member_name) - 1),
               member_expr->member_name);
      expr = member_expr;
      continue;
    }
    if (js_accept_punct(parser, "(")) {
      size_t arg_count = 0;
      js_expr_t *call_expr = js_new_expr(parser);
      if (call_expr == NULL) {
        return NULL;
      }
      call_expr->kind = JS_EXPR_CALL;
      js_copy_text(call_expr->text, sizeof(call_expr->text), expr->text);
      call_expr->left = expr;
      if (!js_accept_punct(parser, ")")) {
        while (1) {
          if (arg_count >= JS_MAX_ARGS) {
            return NULL;
          }
          call_expr->args[arg_count] = js_parse_expression(parser);
          if (call_expr->args[arg_count] == NULL) {
            return NULL;
          }
          arg_count += 1;
          if (js_accept_punct(parser, ")")) {
            break;
          }
          if (!js_accept_punct(parser, ",")) {
            return NULL;
          }
        }
      }
      call_expr->arg_count = arg_count;
      expr = call_expr;
      continue;
    }
    break;
  }

  return expr;
}

static js_expr_t *js_parse_unary(js_parser_t *parser) {
  if (js_accept_punct(parser, "-")) {
    js_expr_t *zero = js_new_expr(parser);
    js_expr_t *expr = js_new_expr(parser);
    if (zero == NULL || expr == NULL) {
      return NULL;
    }
    zero->kind = JS_EXPR_NUMBER;
    snprintf(zero->text, sizeof(zero->text), "0");
    zero->inferred_type = JS_TYPE_INT;
    expr->kind = JS_EXPR_BINARY;
    snprintf(expr->op, sizeof(expr->op), "-");
    expr->left = zero;
    expr->right = js_parse_unary(parser);
    return expr;
  }
  return js_parse_primary(parser);
}

static js_expr_t *js_parse_multiplicative(js_parser_t *parser) {
  js_expr_t *left = js_parse_unary(parser);
  while (1) {
    js_expr_t *expr;
    js_skip_noise(parser);
    if (!(js_accept_punct(parser, "*") || js_accept_punct(parser, "/"))) {
      break;
    }
    expr = js_new_expr(parser);
    if (expr == NULL) {
      return NULL;
    }
    expr->kind = JS_EXPR_BINARY;
    snprintf(expr->op, sizeof(expr->op), "%.*s", (int)parser->tokens[parser->index - 1].length, parser->tokens[parser->index - 1].start);
    expr->left = left;
    expr->right = js_parse_unary(parser);
    left = expr;
  }
  return left;
}

static js_expr_t *js_parse_additive(js_parser_t *parser) {
  js_expr_t *left = js_parse_multiplicative(parser);
  while (1) {
    js_expr_t *expr;
    js_skip_noise(parser);
    if (!(js_accept_punct(parser, "+") || js_accept_punct(parser, "-"))) {
      break;
    }
    expr = js_new_expr(parser);
    if (expr == NULL) {
      return NULL;
    }
    expr->kind = JS_EXPR_BINARY;
    snprintf(expr->op, sizeof(expr->op), "%.*s", (int)parser->tokens[parser->index - 1].length, parser->tokens[parser->index - 1].start);
    expr->left = left;
    expr->right = js_parse_multiplicative(parser);
    left = expr;
  }
  return left;
}

static js_expr_t *js_parse_expression(js_parser_t *parser) {
  return js_parse_additive(parser);
}

static int js_parse_statements(js_parser_t *parser) {
  while (parser->statement_count < JS_MAX_STATEMENTS) {
    js_stmt_t *statement;
    char ident[64];

    js_skip_noise(parser);
    if (js_current(parser)->kind == TOKEN_EOF) {
      return 0;
    }

    statement = &parser->statements[parser->statement_count];
    memset(statement, 0, sizeof(*statement));

    if (js_current(parser)->kind == TOKEN_IDENTIFIER && js_token_matches(js_current(parser), "const")) {
      js_advance(parser);
      if (!js_accept_identifier(parser, statement->name, sizeof(statement->name))) {
        return 1;
      }
      if (!js_accept_punct(parser, "=")) {
        return 1;
      }
      statement->kind = JS_STMT_CONST;
      statement->expr = js_parse_expression(parser);
      if (statement->expr == NULL || !js_accept_punct(parser, ";")) {
        return 1;
      }
      parser->statement_count += 1;
      continue;
    }

    if (js_current(parser)->kind == TOKEN_IDENTIFIER && js_token_matches(js_current(parser), "return")) {
      js_advance(parser);
      statement->kind = JS_STMT_RETURN;
      statement->expr = js_parse_expression(parser);
      if (statement->expr == NULL || !js_accept_punct(parser, ";")) {
        return 1;
      }
      parser->statement_count += 1;
      continue;
    }

    if (!js_accept_identifier(parser, ident, sizeof(ident))) {
      return 1;
    }
    snprintf(statement->object_name, sizeof(statement->object_name), "%s", ident);
    if (!js_accept_punct(parser, ".")) {
      return 1;
    }
    if (!js_accept_identifier(parser, statement->member_name, sizeof(statement->member_name))) {
      return 1;
    }
    if (!js_accept_punct(parser, "=")) {
      return 1;
    }
    statement->kind = JS_STMT_ASSIGN;
    statement->expr = js_parse_expression(parser);
    if (statement->expr == NULL || !js_accept_punct(parser, ";")) {
      return 1;
    }
    parser->statement_count += 1;
  }

  return 1;
}

static void js_symbols_set(js_symbol_table_t *symbols, const char *name, js_type_t type) {
  size_t index;
  for (index = 0; index < symbols->count; ++index) {
    if (strcmp(symbols->symbols[index].name, name) == 0) {
      symbols->symbols[index].type = type;
      return;
    }
  }
  if (symbols->count >= JS_MAX_SYMBOLS) {
    return;
  }
  snprintf(symbols->symbols[symbols->count].name, sizeof(symbols->symbols[symbols->count].name), "%s", name);
  symbols->symbols[symbols->count].type = type;
  symbols->count += 1;
}

static js_type_t js_symbols_get(const js_symbol_table_t *symbols, const char *name) {
  size_t index;
  for (index = 0; index < symbols->count; ++index) {
    if (strcmp(symbols->symbols[index].name, name) == 0) {
      return symbols->symbols[index].type;
    }
  }
  return JS_TYPE_UNKNOWN;
}

static js_type_t js_infer_expr(js_expr_t *expr, const js_symbol_table_t *symbols) {
  if (expr == NULL) {
    return JS_TYPE_UNKNOWN;
  }
  if (expr->inferred_type != JS_TYPE_UNKNOWN) {
    return expr->inferred_type;
  }

  switch (expr->kind) {
    case JS_EXPR_NUMBER:
      return expr->inferred_type;
    case JS_EXPR_IDENTIFIER:
      expr->inferred_type = js_symbols_get(symbols, expr->text);
      return expr->inferred_type;
    case JS_EXPR_MEMBER:
      if (strcmp(expr->text, "Math.PI") == 0) {
        expr->inferred_type = JS_TYPE_DOUBLE;
      } else {
        expr->inferred_type = JS_TYPE_UNKNOWN;
      }
      return expr->inferred_type;
    case JS_EXPR_CALL:
      if (strcmp(expr->text, "Math.sin") == 0 ||
          strcmp(expr->text, "Math.cos") == 0 ||
          strcmp(expr->text, "Math.min") == 0 ||
          strcmp(expr->text, "Math.max") == 0 ||
          strcmp(expr->text, "this.clamp") == 0 ||
          strcmp(expr->text, "this.mapPercentToAngle") == 0) {
        expr->inferred_type = JS_TYPE_DOUBLE;
      } else if (strstr(expr->text, ".toFixed") != NULL) {
        expr->inferred_type = JS_TYPE_STRING;
      }
      return expr->inferred_type;
    case JS_EXPR_BINARY: {
      js_type_t left = js_infer_expr(expr->left, symbols);
      js_type_t right = js_infer_expr(expr->right, symbols);
      if (strcmp(expr->op, "/") == 0) {
        expr->inferred_type = JS_TYPE_DOUBLE;
      } else if (left == JS_TYPE_DOUBLE || right == JS_TYPE_DOUBLE) {
        expr->inferred_type = JS_TYPE_DOUBLE;
      } else {
        expr->inferred_type = JS_TYPE_INT;
      }
      return expr->inferred_type;
    }
  }

  return JS_TYPE_UNKNOWN;
}

static const char *js_type_to_c(js_type_t type) {
  switch (type) {
    case JS_TYPE_INT:
      return "int";
    case JS_TYPE_DOUBLE:
      return "double";
    case JS_TYPE_BOOL:
      return "bool";
    case JS_TYPE_STRING:
      return "const char *";
    case JS_TYPE_VOID:
      return "void";
    case JS_TYPE_UNKNOWN:
    default:
      return "double";
  }
}

const char *js_type_name(js_type_t type) {
  switch (type) {
    case JS_TYPE_INT:
      return "int";
    case JS_TYPE_DOUBLE:
      return "double";
    case JS_TYPE_BOOL:
      return "bool";
    case JS_TYPE_STRING:
      return "string";
    case JS_TYPE_VOID:
      return "void";
    case JS_TYPE_UNKNOWN:
    default:
      return "unknown";
  }
}

static int js_emit_expr(char *buffer, size_t buffer_size, js_expr_t *expr, const js_symbol_table_t *symbols) {
  size_t cursor = strlen(buffer);
  size_t index;

  switch (expr->kind) {
    case JS_EXPR_NUMBER:
      snprintf(buffer + cursor, buffer_size - cursor, "%s", expr->text);
      return 0;
    case JS_EXPR_IDENTIFIER:
      snprintf(buffer + cursor, buffer_size - cursor, "%s", expr->text);
      return 0;
    case JS_EXPR_MEMBER:
      if (strcmp(expr->text, "Math.PI") == 0) {
        snprintf(buffer + cursor, buffer_size - cursor, "NG_MATH_PI");
        return 0;
      }
      return 1;
    case JS_EXPR_BINARY:
      snprintf(buffer + cursor, buffer_size - cursor, "(");
      if (js_emit_expr(buffer, buffer_size, expr->left, symbols) != 0) {
        return 1;
      }
      cursor = strlen(buffer);
      snprintf(buffer + cursor, buffer_size - cursor, " %s ", expr->op);
      if (js_emit_expr(buffer, buffer_size, expr->right, symbols) != 0) {
        return 1;
      }
      cursor = strlen(buffer);
      snprintf(buffer + cursor, buffer_size - cursor, ")");
      return 0;
    case JS_EXPR_CALL:
      if (strcmp(expr->text, "Math.sin") == 0) {
        snprintf(buffer + cursor, buffer_size - cursor, "ng_math_sin(");
      } else if (strcmp(expr->text, "Math.cos") == 0) {
        snprintf(buffer + cursor, buffer_size - cursor, "ng_math_cos(");
      } else if (strcmp(expr->text, "Math.min") == 0) {
        snprintf(buffer + cursor, buffer_size - cursor, "ng_math_min(");
      } else if (strcmp(expr->text, "Math.max") == 0) {
        snprintf(buffer + cursor, buffer_size - cursor, "ng_math_max(");
      } else if (strcmp(expr->text, "this.clamp") == 0) {
        snprintf(buffer + cursor, buffer_size - cursor, "angular_clamp_call(runtime, ");
      } else if (strcmp(expr->text, "this.mapPercentToAngle") == 0) {
        snprintf(buffer + cursor, buffer_size - cursor, "angular_mapPercentToAngle_call(runtime, ");
        if (expr->arg_count != 1) {
          return 1;
        }
        if (js_emit_expr(buffer, buffer_size, expr->args[0], symbols) != 0) {
          return 1;
        }
        snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer), ")");
        return 0;
      } else if (strstr(expr->text, ".toFixed") != NULL) {
        return 1;
      } else {
        return 1;
      }

      for (index = 0; index < expr->arg_count; ++index) {
        if (index > 0) {
          cursor = strlen(buffer);
          snprintf(buffer + cursor, buffer_size - cursor, ", ");
        }
        if (js_emit_expr(buffer, buffer_size, expr->args[index], symbols) != 0) {
          return 1;
        }
      }
      cursor = strlen(buffer);
      snprintf(buffer + cursor, buffer_size - cursor, ")");
      return 0;
  }

  return 1;
}

static int js_emit_statement(const ast_member_t *member,
                             char *buffer,
                             size_t buffer_size,
                             js_stmt_t *statement,
                             const js_symbol_table_t *symbols,
                             size_t temp_index) {
  size_t cursor = strlen(buffer);
  char expr_code[1024];

  expr_code[0] = '\0';

  if (statement->kind == JS_STMT_CONST) {
    js_type_t type = js_infer_expr(statement->expr, symbols);
    if (js_emit_expr(expr_code, sizeof(expr_code), statement->expr, symbols) != 0) {
      return 1;
    }
    snprintf(buffer + cursor,
             buffer_size - cursor,
             "  %s %s = %s;\n",
             js_type_to_c(type),
             statement->name,
             expr_code);
    return 0;
  }

  if (statement->kind == JS_STMT_RETURN) {
    if (js_emit_expr(expr_code, sizeof(expr_code), statement->expr, symbols) != 0) {
      return 1;
    }
    snprintf(buffer + cursor, buffer_size - cursor, "  return %s;\n", expr_code);
    return 0;
  }

  if (statement->kind == JS_STMT_ASSIGN && strcmp(statement->object_name, "this") == 0) {
    if (statement->expr->kind == JS_EXPR_CALL && strstr(statement->expr->text, ".toFixed") != NULL) {
      char local_buffer[64];
      js_expr_t *target_expr = statement->expr->left;
      snprintf(local_buffer, sizeof(local_buffer), "_fmt%zu", temp_index);
      expr_code[0] = '\0';
      if (target_expr == NULL) {
        return 1;
      }
      if (target_expr->kind == JS_EXPR_MEMBER) {
        snprintf(expr_code, sizeof(expr_code), "%s", target_expr->object_name);
      } else if (js_emit_expr(expr_code, sizeof(expr_code), target_expr, symbols) != 0) {
        return 1;
      }
      snprintf(buffer + cursor,
               buffer_size - cursor,
               "  {\n"
               "    char %s[32];\n"
               "    ng_format_fixed_1(%s, %s, sizeof(%s));\n"
               "    ng_runtime_set_string(runtime, \"%s\", %s);\n"
               "  }\n",
               local_buffer,
               expr_code,
               local_buffer,
               local_buffer,
               statement->member_name,
               local_buffer);
      return 0;
    }

    if (js_emit_expr(expr_code, sizeof(expr_code), statement->expr, symbols) != 0) {
      return 1;
    }

    switch (js_infer_expr(statement->expr, symbols)) {
      case JS_TYPE_INT:
        snprintf(buffer + cursor,
                 buffer_size - cursor,
                 "  ng_runtime_set_int(runtime, \"%s\", %s);\n",
                 statement->member_name,
                 expr_code);
        return 0;
      case JS_TYPE_BOOL:
        snprintf(buffer + cursor,
                 buffer_size - cursor,
                 "  ng_runtime_set_bool(runtime, \"%s\", %s);\n",
                 statement->member_name,
                 expr_code);
        return 0;
      case JS_TYPE_DOUBLE:
      case JS_TYPE_UNKNOWN:
        snprintf(buffer + cursor,
                 buffer_size - cursor,
                 "  ng_runtime_set_double(runtime, \"%s\", %s);\n",
                 statement->member_name,
                 expr_code);
        return 0;
      case JS_TYPE_STRING:
        snprintf(buffer + cursor,
                 buffer_size - cursor,
                 "  ng_runtime_set_string(runtime, \"%s\", %s);\n",
                 statement->member_name,
                 expr_code);
        return 0;
      case JS_TYPE_VOID:
        return 1;
    }
  }

  LOG_TRACE("js_emit_statement unsupported member=%s statement_kind=%d\n", member->name, (int)statement->kind);
  return 1;
}

static void js_seed_param_types(const ast_member_t *member, js_symbol_table_t *symbols) {
  size_t index;
  for (index = 0; index < member->param_count; ++index) {
    js_symbols_set(symbols, member->params[index], JS_TYPE_DOUBLE);
  }
}

int js_codegen_compile_method(const ast_component_file_t *component,
                              const ast_member_t *member,
                              js_codegen_result_t *out_result) {
  tokenizer_t tokenizer;
  token_t token;
  js_parser_t parser;
  js_symbol_table_t symbols;
  js_type_t return_type;
  size_t index;
  size_t cursor = 0;

  (void)component;

  memset(out_result, 0, sizeof(*out_result));
  memset(&parser, 0, sizeof(parser));
  memset(&symbols, 0, sizeof(symbols));

  if (member->kind != AST_MEMBER_METHOD || member->body[0] == '\0') {
    return 1;
  }

  tokenizer_init(&tokenizer, member->body, strlen(member->body));
  while (parser.count + 1 < JS_MAX_TOKENS) {
    token = tokenizer_next(&tokenizer);
    parser.tokens[parser.count] = token;
    parser.count += 1;
    if (token.kind == TOKEN_EOF) {
      break;
    }
  }

  if (js_parse_statements(&parser) != 0) {
    LOG_TRACE("js_codegen parse failed for member=%s\n", member->name);
    return 1;
  }

  js_seed_param_types(member, &symbols);
  return_type = JS_TYPE_VOID;

  for (index = 0; index < parser.statement_count; ++index) {
    js_stmt_t *statement = &parser.statements[index];
    if (statement->kind == JS_STMT_CONST) {
      js_symbols_set(&symbols, statement->name, js_infer_expr(statement->expr, &symbols));
    } else if (statement->kind == JS_STMT_RETURN) {
      return_type = js_infer_expr(statement->expr, &symbols);
    }
  }

  if (return_type == JS_TYPE_UNKNOWN) {
    return_type = JS_TYPE_VOID;
  }
  out_result->return_type = return_type;

  cursor += (size_t)snprintf(out_result->prototype + cursor,
                             sizeof(out_result->prototype) - cursor,
                             "%s angular_%s_call(ng_runtime_t *runtime",
                             js_type_to_c(return_type),
                             member->name);
  for (index = 0; index < member->param_count; ++index) {
    cursor += (size_t)snprintf(out_result->prototype + cursor,
                               sizeof(out_result->prototype) - cursor,
                               ", %s %s",
                               js_type_to_c(js_symbols_get(&symbols, member->params[index])),
                               member->params[index]);
  }
  snprintf(out_result->prototype + cursor,
           sizeof(out_result->prototype) - cursor,
           ");");

  cursor = 0;
  cursor += (size_t)snprintf(out_result->definition + cursor,
                             sizeof(out_result->definition) - cursor,
                             "%s angular_%s_call(ng_runtime_t *runtime",
                             js_type_to_c(return_type),
                             member->name);
  for (index = 0; index < member->param_count; ++index) {
    cursor += (size_t)snprintf(out_result->definition + cursor,
                               sizeof(out_result->definition) - cursor,
                               ", %s %s",
                               js_type_to_c(js_symbols_get(&symbols, member->params[index])),
                               member->params[index]);
  }
  cursor += (size_t)snprintf(out_result->definition + cursor, sizeof(out_result->definition) - cursor, ") {\n");
  cursor += (size_t)snprintf(out_result->definition + cursor,
                             sizeof(out_result->definition) - cursor,
                             "  (void)runtime;\n");
  for (index = 0; index < parser.statement_count; ++index) {
    if (js_emit_statement(member,
                          out_result->definition,
                          sizeof(out_result->definition),
                          &parser.statements[index],
                          &symbols,
                          index) != 0) {
      LOG_TRACE("js_codegen emit failed member=%s statement=%zu\n", member->name, index);
      return 1;
    }
  }
  cursor = strlen(out_result->definition);
  snprintf(out_result->definition + cursor, sizeof(out_result->definition) - cursor, "}\n");
  out_result->supported = 1;
  LOG_TRACE("js_codegen compiled member=%s return_type=%s params=%zu\n",
            member->name,
            js_type_name(return_type),
            member->param_count);
  return 0;
}
