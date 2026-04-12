#include "runtime/server_runtime.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *text;
  size_t offset;
} ng_expr_parser_t;

typedef struct {
  const ng_http_request_t *request;
  const ng_server_binding_t *bindings;
  size_t binding_count;
  const ng_server_scope_entry_t *scope;
  const json_data *model;
} ng_eval_context_t;

static void ng_expr_skip_space(ng_expr_parser_t *parser) {
  while (parser->text[parser->offset] == ' ' ||
         parser->text[parser->offset] == '\t' ||
         parser->text[parser->offset] == '\r' ||
         parser->text[parser->offset] == '\n') {
    parser->offset += 1;
  }
}

static int ng_expr_accept(ng_expr_parser_t *parser, char ch) {
  ng_expr_skip_space(parser);
  if (parser->text[parser->offset] == ch) {
    parser->offset += 1;
    return 1;
  }
  return 0;
}

static int ng_expr_peek(ng_expr_parser_t *parser) {
  ng_expr_skip_space(parser);
  return parser->text[parser->offset];
}

static int ng_expr_parse_identifier(ng_expr_parser_t *parser, char *buffer, size_t buffer_size) {
  size_t cursor = 0;

  ng_expr_skip_space(parser);
  if (!(isalpha((unsigned char)parser->text[parser->offset]) || parser->text[parser->offset] == '_' || parser->text[parser->offset] == '$')) {
    return 1;
  }

  while ((isalnum((unsigned char)parser->text[parser->offset]) ||
          parser->text[parser->offset] == '_' ||
          parser->text[parser->offset] == '$') &&
         cursor + 1 < buffer_size) {
    buffer[cursor++] = parser->text[parser->offset++];
  }
  buffer[cursor] = '\0';
  return 0;
}

static int ng_expr_parse_string_literal(ng_expr_parser_t *parser, char *buffer, size_t buffer_size) {
  char quote;
  size_t cursor = 0;

  ng_expr_skip_space(parser);
  quote = parser->text[parser->offset];
  if (quote != '\'' && quote != '"') {
    return 1;
  }
  parser->offset += 1;
  while (parser->text[parser->offset] != '\0' && parser->text[parser->offset] != quote) {
    if (parser->text[parser->offset] == '\\' && parser->text[parser->offset + 1] != '\0') {
      parser->offset += 1;
    }
    if (cursor + 1 >= buffer_size) {
      return 1;
    }
    buffer[cursor++] = parser->text[parser->offset++];
  }
  if (parser->text[parser->offset] != quote) {
    return 1;
  }
  parser->offset += 1;
  buffer[cursor] = '\0';
  return 0;
}

static json_data *ng_server_eval_expr_internal(ng_expr_parser_t *parser, const ng_eval_context_t *context);

json_data *ng_server_json_clone(const json_data *value) {
  int index;
  json_data *copy;

  if (value == NULL) {
    return init_json_null();
  }

  switch (value->type) {
    case JSON_STRING:
      return init_json_string(value->as.string.data);
    case JSON_NUMBER:
      return init_json_number(value->as.number.data);
    case JSON_BOOLEAN:
      return init_json_boolean(value->as.boolean.data);
    case JSON_NULL:
      return init_json_null();
    case JSON_ARRAY:
      copy = init_json_array(0);
      if (copy == NULL) {
        return NULL;
      }
      for (index = 0; index < value->as.array.len; ++index) {
        json_array_add(copy, ng_server_json_clone(&value->as.array.data[index]));
      }
      return copy;
    case JSON_OBJECT:
      copy = init_json_object();
      if (copy == NULL) {
        return NULL;
      }
      for (index = 0; index < value->as.object.len; ++index) {
        json_kvp *pair = value->as.object.pairs[index];
        if (pair != NULL) {
          json_object_add(copy, pair->key, ng_server_json_clone(pair->value));
        }
      }
      return copy;
  }

  return init_json_null();
}

int ng_server_json_truthy(const json_data *value) {
  if (value == NULL) {
    return 0;
  }
  switch (value->type) {
    case JSON_NULL:
      return 0;
    case JSON_BOOLEAN:
      return value->as.boolean.data;
    case JSON_NUMBER:
      return value->as.number.data != 0.0;
    case JSON_STRING:
      return value->as.string.data != NULL && value->as.string.data[0] != '\0';
    case JSON_ARRAY:
      return value->as.array.len > 0;
    case JSON_OBJECT:
      return value->as.object.len > 0;
  }
  return 0;
}

static json_data *ng_server_scope_lookup(const ng_eval_context_t *context, const char *name) {
  const ng_server_scope_entry_t *entry = context->scope;
  size_t index;

  while (entry != NULL) {
    if (strcmp(entry->name, name) == 0) {
      return ng_server_json_clone(entry->value);
    }
    entry = entry->next;
  }

  if (context->model != NULL && context->model->type == JSON_OBJECT) {
    json_data *model_value = get_value((json_data *)context->model, name);
    if (model_value != NULL) {
      return ng_server_json_clone(model_value);
    }
  }

  for (index = 0; index < context->binding_count; ++index) {
    if (strcmp(context->bindings[index].name, name) == 0) {
      ng_expr_parser_t parser = {context->bindings[index].expr_source, 0};
      return ng_server_eval_expr_internal(&parser, context);
    }
  }

  return NULL;
}

static json_data *ng_server_json_get_member_clone(const json_data *value, const char *name) {
  if (value == NULL || value->type != JSON_OBJECT) {
    return init_json_null();
  }
  return ng_server_json_clone(get_value((json_data *)value, name));
}

static int ng_server_value_to_string(const json_data *value, char *buffer, size_t buffer_size) {
  char *json_text;
  if (buffer == NULL || buffer_size == 0) {
    return 1;
  }
  buffer[0] = '\0';
  if (value == NULL) {
    return 0;
  }
  switch (value->type) {
    case JSON_STRING:
      snprintf(buffer, buffer_size, "%s", value->as.string.data != NULL ? value->as.string.data : "");
      return 0;
    case JSON_NUMBER:
      snprintf(buffer, buffer_size, "%.15g", value->as.number.data);
      return 0;
    case JSON_BOOLEAN:
      snprintf(buffer, buffer_size, "%s", value->as.boolean.data ? "true" : "false");
      return 0;
    case JSON_NULL:
      return 0;
    case JSON_ARRAY:
    case JSON_OBJECT:
      json_text = json_tostring((json_data *)value);
      if (json_text == NULL) {
        return 1;
      }
      snprintf(buffer, buffer_size, "%s", json_text);
      free(json_text);
      return 0;
  }
  return 1;
}

static json_data *ng_server_eval_call(const char *target,
                                      json_data **args,
                                      size_t arg_count,
                                      const ng_eval_context_t *context) {
  char key_buffer[256];
  json_data *result = NULL;

  if ((strcmp(target, "req.query") == 0 || strcmp(target, "query") == 0) && arg_count >= 1) {
    ng_server_value_to_string(args[0], key_buffer, sizeof(key_buffer));
    return init_json_string(ng_http_request_query(context->request, key_buffer));
  }
  if ((strcmp(target, "req.param") == 0 || strcmp(target, "param") == 0) && arg_count >= 1) {
    ng_server_value_to_string(args[0], key_buffer, sizeof(key_buffer));
    return init_json_string(ng_http_request_route_param(context->request, key_buffer));
  }
  if ((strcmp(target, "req.body") == 0 || strcmp(target, "body") == 0) && arg_count >= 1) {
    ng_server_value_to_string(args[0], key_buffer, sizeof(key_buffer));
    return init_json_string(ng_http_request_body_field(context->request, key_buffer));
  }
  if ((strcmp(target, "req.header") == 0 || strcmp(target, "header") == 0) && arg_count >= 1) {
    ng_server_value_to_string(args[0], key_buffer, sizeof(key_buffer));
    return init_json_string(ng_http_request_header(context->request, key_buffer));
  }
  if ((strcmp(target, "config.get") == 0 || strcmp(target, "config") == 0) && arg_count >= 1) {
    ng_server_value_to_string(args[0], key_buffer, sizeof(key_buffer));
    return init_json_string(key_buffer);
  }
  if ((strcmp(target, "state.get") == 0 || strcmp(target, "state") == 0) && arg_count >= 1) {
    ng_server_value_to_string(args[0], key_buffer, sizeof(key_buffer));
    return init_json_string(key_buffer);
  }
  if (strcmp(target, "sensor.read") == 0 || strcmp(target, "sensor") == 0) {
    return init_json_number(42);
  }
  if (strcmp(target, "upper") == 0 && arg_count >= 1) {
    size_t index;
    ng_server_value_to_string(args[0], key_buffer, sizeof(key_buffer));
    for (index = 0; key_buffer[index] != '\0'; ++index) {
      key_buffer[index] = (char)toupper((unsigned char)key_buffer[index]);
    }
    return init_json_string(key_buffer);
  }
  if (strcmp(target, "lower") == 0 && arg_count >= 1) {
    size_t index;
    ng_server_value_to_string(args[0], key_buffer, sizeof(key_buffer));
    for (index = 0; key_buffer[index] != '\0'; ++index) {
      key_buffer[index] = (char)tolower((unsigned char)key_buffer[index]);
    }
    return init_json_string(key_buffer);
  }
  if (strcmp(target, "title") == 0 && arg_count >= 1) {
    size_t index;
    int new_word = 1;
    ng_server_value_to_string(args[0], key_buffer, sizeof(key_buffer));
    for (index = 0; key_buffer[index] != '\0'; ++index) {
      if (isspace((unsigned char)key_buffer[index])) {
        new_word = 1;
      } else if (new_word) {
        key_buffer[index] = (char)toupper((unsigned char)key_buffer[index]);
        new_word = 0;
      } else {
        key_buffer[index] = (char)tolower((unsigned char)key_buffer[index]);
      }
    }
    return init_json_string(key_buffer);
  }
  if (strcmp(target, "length") == 0 && arg_count >= 1) {
    switch (args[0] != NULL ? args[0]->type : JSON_NULL) {
      case JSON_ARRAY:
        return init_json_number(args[0]->as.array.len);
      case JSON_OBJECT:
        return init_json_number(args[0]->as.object.len);
      case JSON_STRING:
        return init_json_number(strlen(args[0]->as.string.data != NULL ? args[0]->as.string.data : ""));
      default:
        return init_json_number(0);
    }
  }
  if (strcmp(target, "json") == 0 && arg_count >= 1) {
    char *json_text = json_tostring(args[0]);
    result = init_json_string(json_text != NULL ? json_text : "");
    free(json_text);
    return result;
  }

  return init_json_null();
}

static json_data *ng_server_eval_identifier_path(ng_expr_parser_t *parser, const ng_eval_context_t *context) {
  char identifier[128];
  char member[128];
  char call_target[256];
  json_data *value;

  if (ng_expr_parse_identifier(parser, identifier, sizeof(identifier)) != 0) {
    return NULL;
  }
  snprintf(call_target, sizeof(call_target), "%s", identifier);
  value = ng_server_scope_lookup(context, identifier);
  if (value == NULL) {
    value = init_json_null();
  }

  while (ng_expr_accept(parser, '.')) {
    if (ng_expr_parse_identifier(parser, member, sizeof(member)) != 0) {
      json_free(value);
      return init_json_null();
    }
    snprintf(call_target + strlen(call_target),
             sizeof(call_target) - strlen(call_target),
             ".%s",
             member);
    if (ng_expr_peek(parser) == '(') {
      break;
    }
    {
      json_data *next = ng_server_json_get_member_clone(value, member);
      json_free(value);
      value = next;
    }
  }

  if (ng_expr_accept(parser, '(')) {
    json_data *args[8];
    size_t arg_count = 0;
    while (!ng_expr_accept(parser, ')') && arg_count < sizeof(args) / sizeof(args[0])) {
      args[arg_count] = ng_server_eval_expr_internal(parser, context);
      arg_count += 1;
      if (ng_expr_accept(parser, ')')) {
        break;
      }
      ng_expr_accept(parser, ',');
    }
    json_free(value);
    value = ng_server_eval_call(call_target, args, arg_count, context);
    while (arg_count > 0) {
      arg_count -= 1;
      json_free(args[arg_count]);
    }
  }

  return value;
}

static json_data *ng_server_eval_array(ng_expr_parser_t *parser, const ng_eval_context_t *context) {
  json_data *array = init_json_array(0);
  if (array == NULL) {
    return NULL;
  }
  if (!ng_expr_accept(parser, '[')) {
    return array;
  }
  while (!ng_expr_accept(parser, ']')) {
    json_data *item = ng_server_eval_expr_internal(parser, context);
    json_array_add(array, item);
    if (ng_expr_accept(parser, ']')) {
      break;
    }
    ng_expr_accept(parser, ',');
  }
  return array;
}

static json_data *ng_server_eval_object(ng_expr_parser_t *parser, const ng_eval_context_t *context) {
  json_data *object = init_json_object();
  if (object == NULL) {
    return NULL;
  }
  if (!ng_expr_accept(parser, '{')) {
    return object;
  }
  while (!ng_expr_accept(parser, '}')) {
    char key[128];
    json_data *value;
    if (ng_expr_peek(parser) == '\'' || ng_expr_peek(parser) == '"') {
      if (ng_expr_parse_string_literal(parser, key, sizeof(key)) != 0) {
        json_free(object);
        return NULL;
      }
    } else if (ng_expr_parse_identifier(parser, key, sizeof(key)) != 0) {
      json_free(object);
      return NULL;
    }
    ng_expr_accept(parser, ':');
    value = ng_server_eval_expr_internal(parser, context);
    json_object_add(object, key, value);
    if (ng_expr_accept(parser, '}')) {
      break;
    }
    ng_expr_accept(parser, ',');
  }
  return object;
}

static json_data *ng_server_eval_expr_internal(ng_expr_parser_t *parser, const ng_eval_context_t *context) {
  char string_value[512];
  char number_buffer[64];
  size_t cursor = 0;

  ng_expr_skip_space(parser);
  if (parser->text[parser->offset] == '\'' || parser->text[parser->offset] == '"') {
    if (ng_expr_parse_string_literal(parser, string_value, sizeof(string_value)) != 0) {
      return init_json_null();
    }
    return init_json_string(string_value);
  }
  if (parser->text[parser->offset] == '{') {
    return ng_server_eval_object(parser, context);
  }
  if (parser->text[parser->offset] == '[') {
    return ng_server_eval_array(parser, context);
  }
  if (parser->text[parser->offset] == '(') {
    parser->offset += 1;
    {
      json_data *group = ng_server_eval_expr_internal(parser, context);
      ng_expr_accept(parser, ')');
      return group;
    }
  }
  if (strncmp(parser->text + parser->offset, "true", 4) == 0) {
    parser->offset += 4;
    return init_json_boolean(1);
  }
  if (strncmp(parser->text + parser->offset, "false", 5) == 0) {
    parser->offset += 5;
    return init_json_boolean(0);
  }
  if (strncmp(parser->text + parser->offset, "null", 4) == 0) {
    parser->offset += 4;
    return init_json_null();
  }
  if (isdigit((unsigned char)parser->text[parser->offset]) || parser->text[parser->offset] == '-') {
    while ((isdigit((unsigned char)parser->text[parser->offset]) ||
            parser->text[parser->offset] == '.' ||
            parser->text[parser->offset] == '-') &&
           cursor + 1 < sizeof(number_buffer)) {
      number_buffer[cursor++] = parser->text[parser->offset++];
    }
    number_buffer[cursor] = '\0';
    return init_json_number(strtod(number_buffer, NULL));
  }

  return ng_server_eval_identifier_path(parser, context);
}

json_data *ng_server_eval_expr(const char *expr_source,
                               const ng_http_request_t *request,
                               const ng_server_binding_t *bindings,
                               size_t binding_count,
                               const ng_server_scope_entry_t *scope,
                               const json_data *model) {
  ng_expr_parser_t parser;
  ng_eval_context_t context;

  parser.text = expr_source != NULL ? expr_source : "";
  parser.offset = 0;
  context.request = request;
  context.bindings = bindings;
  context.binding_count = binding_count;
  context.scope = scope;
  context.model = model;
  return ng_server_eval_expr_internal(&parser, &context);
}

static void ng_server_append_html_escaped(stringbuilder *builder, const char *text) {
  size_t index;
  if (text == NULL) {
    return;
  }
  for (index = 0; text[index] != '\0'; ++index) {
    switch (text[index]) {
      case '&': append(builder, "&amp;"); break;
      case '<': append(builder, "&lt;"); break;
      case '>': append(builder, "&gt;"); break;
      case '"': append(builder, "&quot;"); break;
      case '\'': append(builder, "&#39;"); break;
      default: append_byte(builder, text[index]); break;
    }
  }
}

void ng_server_append_escaped_value(stringbuilder *builder, const json_data *value) {
  char buffer[512];
  if (ng_server_value_to_string(value, buffer, sizeof(buffer)) == 0) {
    ng_server_append_html_escaped(builder, buffer);
  }
}

void ng_server_append_raw_value(stringbuilder *builder, const json_data *value) {
  char buffer[512];
  if (ng_server_value_to_string(value, buffer, sizeof(buffer)) == 0) {
    append(builder, buffer);
  }
}

static const ng_template_def_t *ng_server_find_template(const ng_template_def_t *templates,
                                                        size_t template_count,
                                                        const char *name) {
  size_t index;
  for (index = 0; index < template_count; ++index) {
    if (strcmp(templates[index].name, name) == 0) {
      return &templates[index];
    }
  }
  return NULL;
}

static int ng_server_find_matching_end(const ng_template_node_t *nodes,
                                       size_t node_count,
                                       size_t start_index,
                                       size_t *out_else_index,
                                       size_t *out_end_index) {
  size_t index;
  int depth = 0;
  *out_else_index = (size_t)-1;
  *out_end_index = (size_t)-1;

  for (index = start_index + 1; index < node_count; ++index) {
    if (nodes[index].kind == NG_TEMPLATE_NODE_IF_OPEN || nodes[index].kind == NG_TEMPLATE_NODE_FOR_OPEN) {
      depth += 1;
    } else if (nodes[index].kind == NG_TEMPLATE_NODE_END) {
      if (depth == 0) {
        *out_end_index = index;
        return 0;
      }
      depth -= 1;
    } else if (nodes[index].kind == NG_TEMPLATE_NODE_ELSE && depth == 0) {
      *out_else_index = index;
    }
  }
  return 1;
}

static int ng_server_render_nodes(stringbuilder *builder,
                                  const ng_template_def_t *templates,
                                  size_t template_count,
                                  const ng_template_node_t *nodes,
                                  size_t start_index,
                                  size_t end_index,
                                  json_data *model,
                                  const ng_http_request_t *request,
                                  const ng_server_scope_entry_t *scope);

static int ng_server_render_include(stringbuilder *builder,
                                    const ng_template_def_t *templates,
                                    size_t template_count,
                                    const char *text,
                                    json_data *model,
                                    const ng_http_request_t *request,
                                    const ng_server_scope_entry_t *scope) {
  char name[128];
  char expr[512];
  const char *pipe = strchr(text, '|');
  json_data *include_model = model;
  int owns_model = 0;
  const ng_template_def_t *include_template;

  if (pipe == NULL) {
    snprintf(name, sizeof(name), "%s", text);
    expr[0] = '\0';
  } else {
    size_t name_length = (size_t)(pipe - text);
    if (name_length >= sizeof(name)) {
      name_length = sizeof(name) - 1;
    }
    memcpy(name, text, name_length);
    name[name_length] = '\0';
    snprintf(expr, sizeof(expr), "%s", pipe + 1);
  }

  if (expr[0] != '\0') {
    include_model = ng_server_eval_expr(expr, request, NULL, 0, scope, model);
    owns_model = 1;
  }

  include_template = ng_server_find_template(templates, template_count, name);
  if (include_template == NULL) {
    if (owns_model) {
      json_free(include_model);
    }
    return 1;
  }

  if (ng_server_render_nodes(builder,
                             templates,
                             template_count,
                             include_template->nodes,
                             0,
                             include_template->node_count,
                             include_model,
                             request,
                             scope) != 0) {
    if (owns_model) {
      json_free(include_model);
    }
    return 1;
  }

  if (owns_model) {
    json_free(include_model);
  }
  return 0;
}

static int ng_server_render_nodes(stringbuilder *builder,
                                  const ng_template_def_t *templates,
                                  size_t template_count,
                                  const ng_template_node_t *nodes,
                                  size_t start_index,
                                  size_t end_index,
                                  json_data *model,
                                  const ng_http_request_t *request,
                                  const ng_server_scope_entry_t *scope) {
  size_t index = start_index;

  while (index < end_index) {
    const ng_template_node_t *node = &nodes[index];

    if (node->kind == NG_TEMPLATE_NODE_TEXT) {
      append(builder, node->text);
    } else if (node->kind == NG_TEMPLATE_NODE_EXPR || node->kind == NG_TEMPLATE_NODE_RAW_EXPR) {
      json_data *value = ng_server_eval_expr(node->text, request, NULL, 0, scope, model);
      if (value == NULL) {
        value = init_json_null();
      }
      if (node->kind == NG_TEMPLATE_NODE_EXPR) {
        ng_server_append_escaped_value(builder, value);
      } else {
        ng_server_append_raw_value(builder, value);
      }
      json_free(value);
    } else if (node->kind == NG_TEMPLATE_NODE_INCLUDE) {
      if (ng_server_render_include(builder, templates, template_count, node->text, model, request, scope) != 0) {
        return 1;
      }
    } else if (node->kind == NG_TEMPLATE_NODE_IF_OPEN) {
      size_t else_index;
      size_t end_match;
      json_data *condition = ng_server_eval_expr(node->text, request, NULL, 0, scope, model);
      if (ng_server_find_matching_end(nodes, end_index, index, &else_index, &end_match) != 0) {
        json_free(condition);
        return 1;
      }
      if (ng_server_json_truthy(condition)) {
        if (ng_server_render_nodes(builder,
                                   templates,
                                   template_count,
                                   nodes,
                                   index + 1,
                                   else_index != (size_t)-1 ? else_index : end_match,
                                   model,
                                   request,
                                   scope) != 0) {
          json_free(condition);
          return 1;
        }
      } else if (else_index != (size_t)-1) {
        if (ng_server_render_nodes(builder,
                                   templates,
                                   template_count,
                                   nodes,
                                   else_index + 1,
                                   end_match,
                                   model,
                                   request,
                                   scope) != 0) {
          json_free(condition);
          return 1;
        }
      }
      json_free(condition);
      index = end_match;
    } else if (node->kind == NG_TEMPLATE_NODE_FOR_OPEN) {
      size_t else_index;
      size_t end_match;
      const char *pipe = strchr(node->text, '|');
      char item_name[64];
      char expr[256];
      json_data *iterable;
      int iterated = 0;

      if (pipe == NULL) {
        return 1;
      }
      {
        size_t item_length = (size_t)(pipe - node->text);
        if (item_length >= sizeof(item_name)) {
          item_length = sizeof(item_name) - 1;
        }
        memcpy(item_name, node->text, item_length);
        item_name[item_length] = '\0';
      }
      snprintf(expr, sizeof(expr), "%s", pipe + 1);
      if (ng_server_find_matching_end(nodes, end_index, index, &else_index, &end_match) != 0) {
        return 1;
      }
      iterable = ng_server_eval_expr(expr, request, NULL, 0, scope, model);
      if (iterable != NULL && iterable->type == JSON_ARRAY) {
        int array_index;
        for (array_index = 0; array_index < iterable->as.array.len; ++array_index) {
          ng_server_scope_entry_t local_scope;
          local_scope.name = item_name;
          local_scope.value = &iterable->as.array.data[array_index];
          local_scope.next = (ng_server_scope_entry_t *)scope;
          if (ng_server_render_nodes(builder,
                                     templates,
                                     template_count,
                                     nodes,
                                     index + 1,
                                     else_index != (size_t)-1 ? else_index : end_match,
                                     model,
                                     request,
                                     &local_scope) != 0) {
            json_free(iterable);
            return 1;
          }
          iterated = 1;
        }
      }
      if (!iterated && else_index != (size_t)-1) {
        if (ng_server_render_nodes(builder,
                                   templates,
                                   template_count,
                                   nodes,
                                   else_index + 1,
                                   end_match,
                                   model,
                                   request,
                                   scope) != 0) {
          json_free(iterable);
          return 1;
        }
      }
      json_free(iterable);
      index = end_match;
    }

    index += 1;
  }

  return 0;
}

static int ng_server_render_template_to_builder(stringbuilder *builder,
                                                const ng_template_def_t *templates,
                                                size_t template_count,
                                                const ng_template_def_t *template_def,
                                                json_data *model,
                                                const ng_http_request_t *request) {
  return ng_server_render_nodes(builder,
                                templates,
                                template_count,
                                template_def->nodes,
                                0,
                                template_def->node_count,
                                model,
                                request,
                                NULL);
}

int ng_server_render_template_response(const ng_template_def_t *templates,
                                       size_t template_count,
                                       const char *template_name,
                                       json_data *model,
                                       const ng_http_request_t *request,
                                       ng_http_response_t *response) {
  const ng_template_def_t *template_def = ng_server_find_template(templates, template_count, template_name);
  stringbuilder *builder;
  char *text;
  int result;

  if (template_def == NULL || response == NULL) {
    return 1;
  }

  builder = init_builder();
  if (builder == NULL) {
    return 1;
  }

  if (template_def->layout_name != NULL && template_def->layout_name[0] != '\0') {
    stringbuilder *body_builder = init_builder();
    json_data *layout_model;
    const ng_template_def_t *layout_def;
    if (body_builder == NULL) {
      free_builder(builder);
      return 1;
    }
    if (ng_server_render_template_to_builder(body_builder, templates, template_count, template_def, model, request) != 0) {
      free_builder(body_builder);
      free_builder(builder);
      return 1;
    }
    layout_model = model != NULL && model->type == JSON_OBJECT ? ng_server_json_clone(model) : init_json_object();
    json_object_add_string(layout_model, "body", body_builder->data != NULL ? "" : "");
    if (layout_model->type == JSON_OBJECT) {
      json_data *body_value = get_value(layout_model, "body");
      if (body_value != NULL && body_value->type == JSON_STRING) {
        free(body_value->as.string.data);
        body_value->as.string.data = malloc((size_t)body_builder->writtenlen + 1u);
        if (body_value->as.string.data != NULL) {
          memcpy(body_value->as.string.data, body_builder->data, (size_t)body_builder->writtenlen);
          body_value->as.string.data[body_builder->writtenlen] = '\0';
        }
      }
    }
    layout_def = ng_server_find_template(templates, template_count, template_def->layout_name);
    free_builder(body_builder);
    if (layout_def == NULL ||
        ng_server_render_template_to_builder(builder, templates, template_count, layout_def, layout_model, request) != 0) {
      json_free(layout_model);
      free_builder(builder);
      return 1;
    }
    json_free(layout_model);
  } else if (ng_server_render_template_to_builder(builder, templates, template_count, template_def, model, request) != 0) {
    free_builder(builder);
    return 1;
  }

  text = (char *)malloc((size_t)builder->writtenlen + 1u);
  if (text == NULL) {
    free_builder(builder);
    return 1;
  }
  memcpy(text, builder->data, (size_t)builder->writtenlen);
  text[builder->writtenlen] = '\0';
  snprintf(response->content_type, sizeof(response->content_type), "text/html; charset=utf-8");
  result = ng_http_response_set_text(response, text);
  free(text);
  free_builder(builder);
  return result;
}
