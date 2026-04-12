#include "ejs_ir.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int ejs_codegen_append(char *buffer, size_t buffer_size, size_t *cursor, const char *text) {
  int written;

  if (buffer == NULL || cursor == NULL || text == NULL || *cursor >= buffer_size) {
    return 1;
  }

  written = snprintf(buffer + *cursor, buffer_size - *cursor, "%s", text);
  if (written < 0 || (size_t)written >= buffer_size - *cursor) {
    return 1;
  }
  *cursor += (size_t)written;
  return 0;
}

static int ejs_codegen_append_format(char *buffer, size_t buffer_size, size_t *cursor, const char *format, ...) {
  int written;
  va_list args;

  if (buffer == NULL || cursor == NULL || format == NULL || *cursor >= buffer_size) {
    return 1;
  }

  va_start(args, format);
  written = vsnprintf(buffer + *cursor, buffer_size - *cursor, format, args);
  va_end(args);
  if (written < 0 || (size_t)written >= buffer_size - *cursor) {
    return 1;
  }
  *cursor += (size_t)written;
  return 0;
}

static void ejs_codegen_escape_c_string(char *buffer, size_t buffer_size, const char *text) {
  size_t cursor = 0;
  size_t index = 0;

  if (buffer_size == 0) {
    return;
  }

  while (text[index] != '\0' && cursor + 2 < buffer_size) {
    char ch = text[index++];
    if (ch == '\\' || ch == '"') {
      buffer[cursor++] = '\\';
      buffer[cursor++] = ch;
    } else if (ch == '\r') {
      buffer[cursor++] = '\\';
      buffer[cursor++] = 'r';
    } else if (ch == '\n') {
      buffer[cursor++] = '\\';
      buffer[cursor++] = 'n';
    } else {
      buffer[cursor++] = ch;
    }
  }
  buffer[cursor] = '\0';
}

static void ejs_codegen_make_safe_name(char *buffer, size_t buffer_size, const char *name) {
  size_t index;
  size_t cursor = 0;

  if (buffer_size == 0) {
    return;
  }

  for (index = 0; name[index] != '\0' && cursor + 1 < buffer_size; ++index) {
    unsigned char ch = (unsigned char)name[index];
    buffer[cursor++] = (char)(isalnum(ch) || ch == '_' ? ch : '_');
  }
  buffer[cursor] = '\0';
}

static int ejs_codegen_is_string_literal(const char *text) {
  size_t length = strlen(text);
  return length >= 2 &&
         ((text[0] == '\'' && text[length - 1] == '\'') || (text[0] == '"' && text[length - 1] == '"'));
}

static void ejs_codegen_unquote(char *buffer) {
  size_t length = strlen(buffer);
  if (length >= 2 && ((buffer[0] == '\'' && buffer[length - 1] == '\'') || (buffer[0] == '"' && buffer[length - 1] == '"'))) {
    memmove(buffer, buffer + 1, length - 2);
    buffer[length - 2] = '\0';
  }
}

static int ejs_codegen_emit_expr_append(char *buffer,
                                        size_t buffer_size,
                                        size_t *cursor,
                                        const char *expr,
                                        int escape_html) {
  char escaped[1024];
  char literal[512];

  if (ejs_codegen_is_string_literal(expr)) {
    snprintf(literal, sizeof(literal), "%s", expr);
    ejs_codegen_unquote(literal);
    ejs_codegen_escape_c_string(escaped, sizeof(escaped), literal);
    return ejs_codegen_append_format(buffer,
                                     buffer_size,
                                     cursor,
                                     "  angular_sb_append_text(builder, \"%s\");\n",
                                     escaped);
  }

  if (strcmp(expr, "true") == 0 || strcmp(expr, "false") == 0) {
    return ejs_codegen_append_format(buffer,
                                     buffer_size,
                                     cursor,
                                     "  angular_sb_append_text(builder, \"%s\");\n",
                                     expr);
  }

  if ((expr[0] >= '0' && expr[0] <= '9') || expr[0] == '-') {
    return ejs_codegen_append_format(buffer,
                                     buffer_size,
                                     cursor,
                                     "  angular_sb_append_text(builder, \"%s\");\n",
                                     expr);
  }

  return ejs_codegen_append_format(buffer,
                                   buffer_size,
                                   cursor,
                                   "  angular_sb_append_%sslot(builder, locals, \"%s\");\n",
                                   escape_html ? "escaped_" : "raw_",
                                   expr);
}

static int ejs_codegen_emit_if_open(char *buffer, size_t buffer_size, size_t *cursor, const char *expr) {
  if (strcmp(expr, "true") == 0) {
    return ejs_codegen_append(buffer, buffer_size, cursor, "  if (1) {\n");
  }
  if (strcmp(expr, "false") == 0) {
    return ejs_codegen_append(buffer, buffer_size, cursor, "  if (0) {\n");
  }
  if ((expr[0] >= '0' && expr[0] <= '9') || expr[0] == '-') {
    return ejs_codegen_append_format(buffer, buffer_size, cursor, "  if ((%s) != 0) {\n", expr);
  }
  return ejs_codegen_append_format(buffer,
                                   buffer_size,
                                   cursor,
                                   "  if (angular_render_context_is_truthy(locals, \"%s\")) {\n",
                                   expr);
}

int ejs_codegen_emit_source(const ng_ejs_template_set_t *templates, char *buffer, size_t buffer_size) {
  size_t cursor = 0;
  size_t template_index;

  if (buffer == NULL || buffer_size == 0) {
    return 1;
  }

  buffer[0] = '\0';

  if (templates == NULL || templates->template_count == 0) {
    return 0;
  }

  if (ejs_codegen_append(
          buffer,
          buffer_size,
          &cursor,
          "typedef enum {\n"
          "  NG_LOCAL_NULL = 0,\n"
          "  NG_LOCAL_BOOL,\n"
          "  NG_LOCAL_INT,\n"
          "  NG_LOCAL_DOUBLE,\n"
          "  NG_LOCAL_STRING\n"
          "} ng_local_kind_t;\n\n"
          "typedef struct {\n"
          "  const char *name;\n"
          "  ng_local_kind_t kind;\n"
          "  union {\n"
          "    int int_value;\n"
          "    double double_value;\n"
          "    int bool_value;\n"
          "    const char *string_value;\n"
          "  } data;\n"
          "} ng_local_slot_t;\n\n"
          "typedef struct {\n"
          "  ng_local_slot_t slots[64];\n"
          "  size_t slot_count;\n"
          "} ng_render_context_t;\n\n"
          "typedef int (*angular_template_render_fn_t)(const ng_render_context_t *locals,\n"
          "                                           ng_http_response_t *response);\n\n"
          "static void angular_render_context_init(ng_render_context_t *context) {\n"
          "  memset(context, 0, sizeof(*context));\n"
          "}\n\n"
          "static int __attribute__((unused)) angular_render_context_add_string(ng_render_context_t *context, const char *name, const char *value) {\n"
          "  ng_local_slot_t *slot;\n"
          "  if (context->slot_count >= sizeof(context->slots) / sizeof(context->slots[0])) {\n"
          "    return 1;\n"
          "  }\n"
          "  slot = &context->slots[context->slot_count++];\n"
          "  slot->name = name;\n"
          "  slot->kind = NG_LOCAL_STRING;\n"
          "  slot->data.string_value = value;\n"
          "  return 0;\n"
          "}\n\n"
          "static int __attribute__((unused)) angular_render_context_add_int(ng_render_context_t *context, const char *name, int value) {\n"
          "  ng_local_slot_t *slot;\n"
          "  if (context->slot_count >= sizeof(context->slots) / sizeof(context->slots[0])) {\n"
          "    return 1;\n"
          "  }\n"
          "  slot = &context->slots[context->slot_count++];\n"
          "  slot->name = name;\n"
          "  slot->kind = NG_LOCAL_INT;\n"
          "  slot->data.int_value = value;\n"
          "  return 0;\n"
          "}\n\n"
          "static int __attribute__((unused)) angular_render_context_add_double(ng_render_context_t *context, const char *name, double value) {\n"
          "  ng_local_slot_t *slot;\n"
          "  if (context->slot_count >= sizeof(context->slots) / sizeof(context->slots[0])) {\n"
          "    return 1;\n"
          "  }\n"
          "  slot = &context->slots[context->slot_count++];\n"
          "  slot->name = name;\n"
          "  slot->kind = NG_LOCAL_DOUBLE;\n"
          "  slot->data.double_value = value;\n"
          "  return 0;\n"
          "}\n\n"
          "static int __attribute__((unused)) angular_render_context_add_bool(ng_render_context_t *context, const char *name, int value) {\n"
          "  ng_local_slot_t *slot;\n"
          "  if (context->slot_count >= sizeof(context->slots) / sizeof(context->slots[0])) {\n"
          "    return 1;\n"
          "  }\n"
          "  slot = &context->slots[context->slot_count++];\n"
          "  slot->name = name;\n"
          "  slot->kind = NG_LOCAL_BOOL;\n"
          "  slot->data.bool_value = value;\n"
          "  return 0;\n"
          "}\n\n"
          "static const ng_local_slot_t *__attribute__((unused)) angular_render_context_find(const ng_render_context_t *context, const char *name) {\n"
          "  size_t index;\n"
          "  for (index = 0; index < context->slot_count; ++index) {\n"
          "    if (context->slots[index].name != NULL && strcmp(context->slots[index].name, name) == 0) {\n"
          "      return &context->slots[index];\n"
          "    }\n"
          "  }\n"
          "  return NULL;\n"
          "}\n\n"
          "static int __attribute__((unused)) angular_render_context_is_truthy(const ng_render_context_t *context, const char *name) {\n"
          "  const ng_local_slot_t *slot = angular_render_context_find(context, name);\n"
          "  if (slot == NULL) {\n"
          "    return 0;\n"
          "  }\n"
          "  switch (slot->kind) {\n"
          "    case NG_LOCAL_BOOL:\n"
          "      return slot->data.bool_value != 0;\n"
          "    case NG_LOCAL_INT:\n"
          "      return slot->data.int_value != 0;\n"
          "    case NG_LOCAL_DOUBLE:\n"
          "      return slot->data.double_value != 0.0;\n"
          "    case NG_LOCAL_STRING:\n"
          "      return slot->data.string_value != NULL && slot->data.string_value[0] != '\\0';\n"
          "    case NG_LOCAL_NULL:\n"
          "    default:\n"
          "      return 0;\n"
          "  }\n"
          "}\n\n"
          "static void angular_sb_append_text(stringbuilder *builder, const char *text) {\n"
          "  append(builder, text != NULL ? text : \"\");\n"
          "}\n\n"
          "static void __attribute__((unused)) angular_sb_append_html_escaped(stringbuilder *builder, const char *text) {\n"
          "  size_t index;\n"
          "  if (text == NULL) {\n"
          "    return;\n"
          "  }\n"
          "  for (index = 0; text[index] != '\\0'; ++index) {\n"
          "    switch (text[index]) {\n"
          "      case '&': append(builder, \"&amp;\"); break;\n"
          "      case '<': append(builder, \"&lt;\"); break;\n"
          "      case '>': append(builder, \"&gt;\"); break;\n"
          "      case '\"': append(builder, \"&quot;\"); break;\n"
          "      case '\\'': append(builder, \"&#39;\"); break;\n"
          "      default: append_byte(builder, text[index]); break;\n"
          "    }\n"
          "  }\n"
          "}\n\n"
          "static void __attribute__((unused)) angular_sb_append_slot_value(stringbuilder *builder, const ng_local_slot_t *slot) {\n"
          "  char number_buffer[64];\n"
          "  if (slot == NULL) {\n"
          "    return;\n"
          "  }\n"
          "  switch (slot->kind) {\n"
          "    case NG_LOCAL_STRING:\n"
          "      angular_sb_append_text(builder, slot->data.string_value);\n"
          "      return;\n"
          "    case NG_LOCAL_BOOL:\n"
          "      angular_sb_append_text(builder, slot->data.bool_value ? \"true\" : \"false\");\n"
          "      return;\n"
          "    case NG_LOCAL_INT:\n"
          "      snprintf(number_buffer, sizeof(number_buffer), \"%d\", slot->data.int_value);\n"
          "      angular_sb_append_text(builder, number_buffer);\n"
          "      return;\n"
          "    case NG_LOCAL_DOUBLE:\n"
          "      snprintf(number_buffer, sizeof(number_buffer), \"%.6g\", slot->data.double_value);\n"
          "      angular_sb_append_text(builder, number_buffer);\n"
          "      return;\n"
          "    case NG_LOCAL_NULL:\n"
          "    default:\n"
          "      return;\n"
          "  }\n"
          "}\n\n"
          "static void __attribute__((unused)) angular_sb_append_escaped_slot(stringbuilder *builder, const ng_render_context_t *context, const char *name) {\n"
          "  const ng_local_slot_t *slot = angular_render_context_find(context, name);\n"
          "  char number_buffer[64];\n"
          "  if (slot == NULL) {\n"
          "    return;\n"
          "  }\n"
          "  if (slot->kind == NG_LOCAL_STRING) {\n"
          "    angular_sb_append_html_escaped(builder, slot->data.string_value);\n"
          "    return;\n"
          "  }\n"
          "  switch (slot->kind) {\n"
          "    case NG_LOCAL_BOOL:\n"
          "      angular_sb_append_text(builder, slot->data.bool_value ? \"true\" : \"false\");\n"
          "      return;\n"
          "    case NG_LOCAL_INT:\n"
          "      snprintf(number_buffer, sizeof(number_buffer), \"%d\", slot->data.int_value);\n"
          "      angular_sb_append_text(builder, number_buffer);\n"
          "      return;\n"
          "    case NG_LOCAL_DOUBLE:\n"
          "      snprintf(number_buffer, sizeof(number_buffer), \"%.6g\", slot->data.double_value);\n"
          "      angular_sb_append_text(builder, number_buffer);\n"
          "      return;\n"
          "    case NG_LOCAL_NULL:\n"
          "    case NG_LOCAL_STRING:\n"
          "    default:\n"
          "      return;\n"
          "  }\n"
          "}\n\n"
          "static void __attribute__((unused)) angular_sb_append_raw_slot(stringbuilder *builder, const ng_render_context_t *context, const char *name) {\n"
          "  angular_sb_append_slot_value(builder, angular_render_context_find(context, name));\n"
          "}\n\n") != 0) {
    return 1;
  }

  for (template_index = 0; template_index < templates->template_count; ++template_index) {
    const ng_ejs_template_t *template_file = &templates->templates[template_index];
    char safe_name[128];
    size_t node_index;

    ejs_codegen_make_safe_name(safe_name, sizeof(safe_name), template_file->name);
    if (ejs_codegen_append_format(buffer,
                                  buffer_size,
                                  &cursor,
                                  "static int angular_render_%s_template(const ng_render_context_t *locals,\n"
                                  "                                        ng_http_response_t *response) {\n"
                                  "  stringbuilder *builder = init_builder();\n"
                                  "  char *rendered;\n"
                                  "  int set_result;\n"
                                  "  if (builder == NULL) {\n"
                                  "    response->status_code = 500;\n"
                                  "    return 0;\n"
                                  "  }\n",
                                  safe_name) != 0) {
      return 1;
    }

    for (node_index = 0; node_index < template_file->node_count; ++node_index) {
      const ng_ejs_node_t *node = &template_file->nodes[node_index];
      char escaped_text[2048];

      switch (node->kind) {
        case NG_EJS_NODE_TEXT:
          ejs_codegen_escape_c_string(escaped_text, sizeof(escaped_text), node->text);
          if (ejs_codegen_append_format(buffer,
                                        buffer_size,
                                        &cursor,
                                        "  angular_sb_append_text(builder, \"%s\");\n",
                                        escaped_text) != 0) {
            return 1;
          }
          break;
        case NG_EJS_NODE_EXPR:
          if (ejs_codegen_emit_expr_append(buffer, buffer_size, &cursor, node->text, 1) != 0) {
            return 1;
          }
          break;
        case NG_EJS_NODE_RAW_EXPR:
          if (ejs_codegen_emit_expr_append(buffer, buffer_size, &cursor, node->text, 0) != 0) {
            return 1;
          }
          break;
        case NG_EJS_NODE_IF_OPEN:
          if (ejs_codegen_emit_if_open(buffer, buffer_size, &cursor, node->text) != 0) {
            return 1;
          }
          break;
        case NG_EJS_NODE_IF_CLOSE:
          if (ejs_codegen_append(buffer, buffer_size, &cursor, "  }\n") != 0) {
            return 1;
          }
          break;
      }
    }

    if (ejs_codegen_append(
            buffer,
            buffer_size,
            &cursor,
            "  rendered = (char *)malloc((size_t)builder->writtenlen + 1u);\n"
            "  if (rendered == NULL) {\n"
            "    free_builder(builder);\n"
            "    response->status_code = 500;\n"
            "    return 0;\n"
            "  }\n"
            "  memcpy(rendered, builder->data, (size_t)builder->writtenlen);\n"
            "  rendered[builder->writtenlen] = '\\0';\n"
            "  set_result = ng_http_response_set_text(response, rendered);\n"
            "  free(rendered);\n"
            "  free_builder(builder);\n"
            "  if (set_result != 0) {\n"
            "    response->status_code = 500;\n"
            "  }\n"
            "  return 0;\n"
            "}\n\n") != 0) {
      return 1;
    }
  }

  return 0;
}
