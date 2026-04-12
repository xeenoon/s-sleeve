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

static const char *ejs_codegen_node_kind_name(ng_ejs_node_kind_t kind) {
  switch (kind) {
    case NG_EJS_NODE_TEXT:
      return "NG_TEMPLATE_NODE_TEXT";
    case NG_EJS_NODE_EXPR:
      return "NG_TEMPLATE_NODE_EXPR";
    case NG_EJS_NODE_RAW_EXPR:
      return "NG_TEMPLATE_NODE_RAW_EXPR";
    case NG_EJS_NODE_IF_OPEN:
      return "NG_TEMPLATE_NODE_IF_OPEN";
    case NG_EJS_NODE_ELSE:
      return "NG_TEMPLATE_NODE_ELSE";
    case NG_EJS_NODE_END:
      return "NG_TEMPLATE_NODE_END";
    case NG_EJS_NODE_FOR_OPEN:
      return "NG_TEMPLATE_NODE_FOR_OPEN";
    case NG_EJS_NODE_INCLUDE:
      return "NG_TEMPLATE_NODE_INCLUDE";
  }
  return "NG_TEMPLATE_NODE_TEXT";
}

int ejs_codegen_emit_source(const ng_ejs_template_set_t *templates, char *buffer, size_t buffer_size) {
  size_t cursor = 0;
  size_t template_index;

  if (buffer == NULL || buffer_size == 0) {
    return 1;
  }

  buffer[0] = '\0';

  if (templates == NULL || templates->template_count == 0) {
    return ejs_codegen_append(buffer,
                              buffer_size,
                              &cursor,
                              "static const ng_template_def_t g_angular_templates[1] = {{0}};\n"
                              "static const size_t g_angular_template_count = 0;\n\n");
  }

  for (template_index = 0; template_index < templates->template_count; ++template_index) {
    const ng_ejs_template_t *template_file = &templates->templates[template_index];
    char safe_name[128];
    size_t node_index;

    ejs_codegen_make_safe_name(safe_name, sizeof(safe_name), template_file->name);
    if (ejs_codegen_append_format(buffer,
                                  buffer_size,
                                  &cursor,
                                  "static const ng_template_node_t g_template_nodes_%s[] = {\n",
                                  safe_name) != 0) {
      return 1;
    }

    for (node_index = 0; node_index < template_file->node_count; ++node_index) {
      char escaped_text[2048];
      const ng_ejs_node_t *node = &template_file->nodes[node_index];

      ejs_codegen_escape_c_string(escaped_text, sizeof(escaped_text), node->text);
      if (ejs_codegen_append_format(buffer,
                                    buffer_size,
                                    &cursor,
                                    "  { %s, \"%s\" },\n",
                                    ejs_codegen_node_kind_name(node->kind),
                                    escaped_text) != 0) {
        return 1;
      }
    }

    if (ejs_codegen_append(buffer, buffer_size, &cursor, "};\n\n") != 0) {
      return 1;
    }
  }

  if (ejs_codegen_append(buffer,
                         buffer_size,
                         &cursor,
                         "static const ng_template_def_t g_angular_templates[] = {\n") != 0) {
    return 1;
  }

  for (template_index = 0; template_index < templates->template_count; ++template_index) {
    const ng_ejs_template_t *template_file = &templates->templates[template_index];
    char safe_name[128];
    char escaped_name[256];
    char escaped_layout[256];

    ejs_codegen_make_safe_name(safe_name, sizeof(safe_name), template_file->name);
    ejs_codegen_escape_c_string(escaped_name, sizeof(escaped_name), template_file->name);
    ejs_codegen_escape_c_string(escaped_layout, sizeof(escaped_layout), template_file->layout_name);
    if (ejs_codegen_append_format(buffer,
                                  buffer_size,
                                  &cursor,
                                  "  { \"%s\", \"%s\", g_template_nodes_%s, sizeof(g_template_nodes_%s) / sizeof(g_template_nodes_%s[0]) },\n",
                                  escaped_name,
                                  escaped_layout,
                                  safe_name,
                                  safe_name,
                                  safe_name) != 0) {
      return 1;
    }
  }

  if (ejs_codegen_append_format(buffer,
                                buffer_size,
                                &cursor,
                                "};\n"
                                "static const size_t g_angular_template_count = sizeof(g_angular_templates) / sizeof(g_angular_templates[0]);\n\n") != 0) {
    return 1;
  }

  return 0;
}
