#include "server_ir.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int server_codegen_append(char *buffer, size_t buffer_size, size_t *cursor, const char *text) {
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

static int server_codegen_append_format(char *buffer, size_t buffer_size, size_t *cursor, const char *format, ...) {
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

static void server_codegen_make_safe_name(char *buffer, size_t buffer_size, const char *name) {
  size_t index;
  size_t cursor = 0;

  if (buffer_size == 0) {
    return;
  }

  for (index = 0; name[index] != '\0' && cursor + 1 < buffer_size; ++index) {
    char ch = name[index];
    if ((ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') ||
        ch == '_') {
      buffer[cursor++] = ch;
    } else {
      buffer[cursor++] = '_';
    }
  }
  buffer[cursor] = '\0';
}

static void server_codegen_escape_c_string(char *buffer, size_t buffer_size, const char *text) {
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

static int server_codegen_emit_locals(const ng_server_route_ir_t *route,
                                      char *buffer,
                                      size_t buffer_size,
                                      size_t *cursor) {
  size_t index;

  for (index = 0; index < route->local_count; ++index) {
    const ng_server_local_ir_t *local = &route->locals[index];
    char escaped_name[160];
    char escaped_value[512];
    server_codegen_escape_c_string(escaped_name, sizeof(escaped_name), local->name);
    server_codegen_escape_c_string(escaped_value, sizeof(escaped_value), local->value);
    switch (local->kind) {
      case NG_LOCAL_STRING:
        if (server_codegen_append_format(buffer,
                                         buffer_size,
                                         cursor,
                                         "  angular_render_context_add_string(&locals, \"%s\", \"%s\");\n",
                                         escaped_name,
                                         escaped_value) != 0) {
          return 1;
        }
        break;
      case NG_LOCAL_INT:
        if (server_codegen_append_format(buffer,
                                         buffer_size,
                                         cursor,
                                         "  angular_render_context_add_int(&locals, \"%s\", %s);\n",
                                         escaped_name,
                                         local->value) != 0) {
          return 1;
        }
        break;
      case NG_LOCAL_DOUBLE:
        if (server_codegen_append_format(buffer,
                                         buffer_size,
                                         cursor,
                                         "  angular_render_context_add_double(&locals, \"%s\", %s);\n",
                                         escaped_name,
                                         local->value) != 0) {
          return 1;
        }
        break;
      case NG_LOCAL_BOOL:
        if (server_codegen_append_format(buffer,
                                         buffer_size,
                                         cursor,
                                         "  angular_render_context_add_bool(&locals, \"%s\", %s);\n",
                                         escaped_name,
                                         strcmp(local->value, "true") == 0 ? "1" : "0") != 0) {
          return 1;
        }
        break;
      case NG_LOCAL_NULL:
      default:
        break;
    }
  }

  return 0;
}

static int server_codegen_emit_json_builder(const ng_server_route_ir_t *route,
                                            char *buffer,
                                            size_t buffer_size,
                                            size_t *cursor) {
  size_t index;

  if (server_codegen_append(buffer,
                            buffer_size,
                            cursor,
                            "  json_data *root = init_json_object();\n"
                            "  char *json_text = NULL;\n"
                            "  if (root == NULL) {\n"
                            "    response->status_code = 500;\n"
                            "    angular_http_write_error_json(response, \"json allocation failed\");\n"
                            "    return 0;\n"
                            "  }\n") != 0) {
    return 1;
  }

  for (index = 0; index < route->local_count; ++index) {
    const ng_server_local_ir_t *local = &route->locals[index];
    char escaped_name[160];
    char escaped_value[512];
    server_codegen_escape_c_string(escaped_name, sizeof(escaped_name), local->name);
    server_codegen_escape_c_string(escaped_value, sizeof(escaped_value), local->value);
    switch (local->kind) {
      case NG_LOCAL_STRING:
        if (server_codegen_append_format(buffer,
                                         buffer_size,
                                         cursor,
                                         "  json_object_add_string(root, \"%s\", \"%s\");\n",
                                         escaped_name,
                                         escaped_value) != 0) {
          return 1;
        }
        break;
      case NG_LOCAL_INT:
      case NG_LOCAL_DOUBLE:
        if (server_codegen_append_format(buffer,
                                         buffer_size,
                                         cursor,
                                         "  json_object_add_number(root, \"%s\", %s);\n",
                                         escaped_name,
                                         local->value) != 0) {
          return 1;
        }
        break;
      case NG_LOCAL_BOOL:
        if (server_codegen_append_format(buffer,
                                         buffer_size,
                                         cursor,
                                         "  json_object_add_boolean(root, \"%s\", %s);\n",
                                         escaped_name,
                                         strcmp(local->value, "true") == 0 ? "true" : "false") != 0) {
          return 1;
        }
        break;
      case NG_LOCAL_NULL:
        if (server_codegen_append_format(buffer,
                                         buffer_size,
                                         cursor,
                                         "  json_object_add_null(root, \"%s\");\n",
                                         escaped_name) != 0) {
          return 1;
        }
        break;
    }
  }

  return server_codegen_append(buffer,
                               buffer_size,
                               cursor,
                               "  json_text = json_tostring(root);\n"
                               "  json_free(root);\n"
                               "  if (json_text == NULL) {\n"
                               "    response->status_code = 500;\n"
                               "    angular_http_write_error_json(response, \"json serialization failed\");\n"
                               "    return 0;\n"
                               "  }\n"
                               "  snprintf(response->content_type, sizeof(response->content_type), \"application/json; charset=utf-8\");\n"
                               "  ng_http_response_set_text(response, json_text);\n"
                               "  free(json_text);\n"
                               "  return 0;\n");
}

int server_codegen_emit(const ng_server_route_set_t *routes,
                        const ng_ejs_template_set_t *templates,
                        size_t route_offset,
                        ng_server_codegen_result_t *out_result) {
  size_t route_cursor = 0;
  size_t init_cursor = 0;
  size_t index;

  if (out_result == NULL) {
    return 1;
  }

  memset(out_result, 0, sizeof(*out_result));
  out_result->route_count = routes != NULL ? routes->route_count : 0;

  if (ejs_codegen_emit_source(templates, out_result->support_source, sizeof(out_result->support_source)) != 0) {
    return 1;
  }

  if (routes == NULL || routes->route_count == 0) {
    return 0;
  }

  for (index = 0; index < routes->route_count; ++index) {
    const ng_server_route_ir_t *route = &routes->routes[index];
    char escaped_path[512];
    char escaped_text[4096];

    if (server_codegen_append_format(out_result->route_source,
                                     sizeof(out_result->route_source),
                                     &route_cursor,
                                     "static int angular_backend_route_%zu(void *context,\n"
                                     "                                  const ng_http_request_t *request,\n"
                                     "                                  ng_http_response_t *response) {\n"
                                     "  (void)context;\n"
                                     "  (void)request;\n"
                                     "  response->status_code = %d;\n",
                                     index,
                                     route->status_code) != 0) {
      return 1;
    }

    switch (route->response_kind) {
      case NG_RESPONSE_TEXT:
        server_codegen_escape_c_string(escaped_text, sizeof(escaped_text), route->response_text);
        if (server_codegen_append_format(out_result->route_source,
                                         sizeof(out_result->route_source),
                                         &route_cursor,
                                         "  snprintf(response->content_type, sizeof(response->content_type), \"text/plain; charset=utf-8\");\n"
                                         "  ng_http_response_set_text(response, \"%s\");\n"
                                         "  return 0;\n",
                                         escaped_text) != 0) {
          return 1;
        }
        break;
      case NG_RESPONSE_JSON:
        if (server_codegen_emit_json_builder(route,
                                             out_result->route_source,
                                             sizeof(out_result->route_source),
                                             &route_cursor) != 0) {
          return 1;
        }
        break;
      case NG_RESPONSE_RENDER: {
        char safe_template[128];
        server_codegen_make_safe_name(safe_template, sizeof(safe_template), route->template_name);
        if (server_codegen_append(out_result->route_source,
                                  sizeof(out_result->route_source),
                                  &route_cursor,
                                  "  ng_render_context_t locals;\n"
                                  "  angular_render_context_init(&locals);\n"
                                  "  snprintf(response->content_type, sizeof(response->content_type), \"text/html; charset=utf-8\");\n") != 0) {
          return 1;
        }
        if (server_codegen_emit_locals(route,
                                       out_result->route_source,
                                       sizeof(out_result->route_source),
                                       &route_cursor) != 0) {
          return 1;
        }
        if (server_codegen_append_format(out_result->route_source,
                                         sizeof(out_result->route_source),
                                         &route_cursor,
                                         "  return angular_render_%s_template(&locals, response);\n",
                                         safe_template) != 0) {
          return 1;
        }
        break;
      }
    }

    if (server_codegen_append(out_result->route_source,
                              sizeof(out_result->route_source),
                              &route_cursor,
                              "}\n\n") != 0) {
      return 1;
    }

    if (server_codegen_append_format(out_result->route_init,
                                     sizeof(out_result->route_init),
                                     &init_cursor,
                                     "  service->routes[%zu].method = \"%s\";\n"
                                     "  service->routes[%zu].path = \"%s\";\n"
                                     "  service->routes[%zu].handler = angular_backend_route_%zu;\n"
                                     "  service->routes[%zu].context = service;\n",
                                     route_offset + index,
                                     route->method == NG_SERVER_METHOD_POST ? "POST" : "GET",
                                     route_offset + index,
                                     (server_codegen_escape_c_string(escaped_path, sizeof(escaped_path), route->path), escaped_path),
                                     route_offset + index,
                                     index,
                                     route_offset + index) != 0) {
      return 1;
    }
  }

  return 0;
}
