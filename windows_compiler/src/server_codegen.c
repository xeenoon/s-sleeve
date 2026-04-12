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

static int server_codegen_emit_bindings(const ng_server_route_ir_t *route,
                                        size_t route_index,
                                        char *buffer,
                                        size_t buffer_size,
                                        size_t *cursor) {
  size_t binding_index;

  if (route->binding_count == 0) {
    return server_codegen_append_format(buffer,
                                        buffer_size,
                                        cursor,
                                        "static const ng_server_binding_t g_route_%zu_bindings[1] = {{NULL, NULL}};\n\n",
                                        route_index);
  }

  if (server_codegen_append_format(buffer,
                                   buffer_size,
                                   cursor,
                                   "static const ng_server_binding_t g_route_%zu_bindings[] = {\n",
                                   route_index) != 0) {
    return 1;
  }

  for (binding_index = 0; binding_index < route->binding_count; ++binding_index) {
    char escaped_name[160];
    char escaped_expr[8192];
    server_codegen_escape_c_string(escaped_name, sizeof(escaped_name), route->bindings[binding_index].name);
    server_codegen_escape_c_string(escaped_expr, sizeof(escaped_expr), route->bindings[binding_index].expr_source);
    if (server_codegen_append_format(buffer,
                                     buffer_size,
                                     cursor,
                                     "  { \"%s\", \"%s\" },\n",
                                     escaped_name,
                                     escaped_expr) != 0) {
      return 1;
    }
  }

  return server_codegen_append(buffer, buffer_size, cursor, "};\n\n");
}

static int server_codegen_emit_json_response(char *buffer,
                                             size_t buffer_size,
                                             size_t *cursor) {
  return server_codegen_append(buffer,
                               buffer_size,
                               cursor,
                               "  response->status_code = status_code;\n"
                               "  snprintf(response->content_type, sizeof(response->content_type), \"application/json; charset=utf-8\");\n"
                               "  value = ng_server_eval_expr(response_expr, request, bindings, binding_count, NULL, NULL);\n"
                               "  if (value == NULL) {\n"
                               "    response->status_code = 500;\n"
                               "    angular_http_write_error_json(response, \"json evaluation failed\");\n"
                               "    return 0;\n"
                               "  }\n"
                               "  text = json_tostring(value);\n"
                               "  json_free(value);\n"
                               "  if (text == NULL) {\n"
                               "    response->status_code = 500;\n"
                               "    angular_http_write_error_json(response, \"json serialization failed\");\n"
                               "    return 0;\n"
                               "  }\n"
                               "  ng_http_response_set_text(response, text);\n"
                               "  free(text);\n"
                               "  return 0;\n");
}

static int server_codegen_emit_text_response(char *buffer,
                                             size_t buffer_size,
                                             size_t *cursor) {
  return server_codegen_append(buffer,
                               buffer_size,
                               cursor,
                               "  response->status_code = status_code;\n"
                               "  snprintf(response->content_type, sizeof(response->content_type), \"text/plain; charset=utf-8\");\n"
                               "  value = ng_server_eval_expr(response_expr, request, bindings, binding_count, NULL, NULL);\n"
                               "  if (value == NULL) {\n"
                               "    response->status_code = 500;\n"
                               "    angular_http_write_error_json(response, \"text evaluation failed\");\n"
                               "    return 0;\n"
                               "  }\n"
                               "  text = json_tostring(value);\n"
                               "  if (text != NULL && value->type == JSON_STRING) {\n"
                               "    free(text);\n"
                               "    text = (char *)malloc(strlen(value->as.string.data != NULL ? value->as.string.data : \"\") + 1u);\n"
                               "    if (text != NULL) {\n"
                               "      strcpy(text, value->as.string.data != NULL ? value->as.string.data : \"\");\n"
                               "    }\n"
                               "  }\n"
                               "  json_free(value);\n"
                               "  if (text == NULL) {\n"
                               "    response->status_code = 500;\n"
                               "    angular_http_write_error_json(response, \"text serialization failed\");\n"
                               "    return 0;\n"
                               "  }\n"
                               "  ng_http_response_set_text(response, text);\n"
                               "  free(text);\n"
                               "  return 0;\n");
}

int server_codegen_emit(const ng_server_route_set_t *routes,
                        const ng_ejs_template_set_t *templates,
                        size_t route_offset,
                        ng_server_codegen_result_t *out_result) {
  size_t route_cursor = 0;
  size_t init_cursor = 0;
  size_t support_cursor = 0;
  size_t index;

  if (out_result == NULL) {
    return 1;
  }

  memset(out_result, 0, sizeof(*out_result));
  out_result->route_count = routes != NULL ? routes->route_count : 0;

  if (ejs_codegen_emit_source(templates, out_result->support_source, sizeof(out_result->support_source)) != 0) {
    return 1;
  }
  support_cursor = strlen(out_result->support_source);

  if (server_codegen_append(out_result->support_source,
                            sizeof(out_result->support_source),
                            &support_cursor,
                            "static json_data *angular_server_model_from_expr(const char *expr,\n"
                            "                                                const ng_http_request_t *request,\n"
                            "                                                const ng_server_binding_t *bindings,\n"
                            "                                                size_t binding_count) {\n"
                            "  json_data *model = ng_server_eval_expr(expr, request, bindings, binding_count, NULL, NULL);\n"
                            "  if (model == NULL) {\n"
                            "    return init_json_object();\n"
                            "  }\n"
                            "  if (model->type != JSON_OBJECT) {\n"
                            "    json_data *wrapped = init_json_object();\n"
                            "    if (wrapped != NULL) {\n"
                            "      json_object_add(wrapped, \"value\", model);\n"
                            "      return wrapped;\n"
                            "    }\n"
                            "    json_free(model);\n"
                            "    return init_json_object();\n"
                            "  }\n"
                            "  return model;\n"
                            "}\n\n") != 0) {
    return 1;
  }

  if (routes == NULL || routes->route_count == 0) {
    return 0;
  }

  for (index = 0; index < routes->route_count; ++index) {
    const ng_server_route_ir_t *route = &routes->routes[index];
    char escaped_path[512];
    char escaped_response_expr[8192];
    char escaped_model_expr[8192];
    char escaped_template[256];

    if (server_codegen_emit_bindings(route,
                                     index,
                                     out_result->support_source,
                                     sizeof(out_result->support_source),
                                     &support_cursor) != 0) {
      return 1;
    }

    server_codegen_escape_c_string(escaped_response_expr, sizeof(escaped_response_expr), route->response_expr);
    server_codegen_escape_c_string(escaped_model_expr, sizeof(escaped_model_expr), route->model_expr);
    server_codegen_escape_c_string(escaped_template, sizeof(escaped_template), route->template_name);

    if (server_codegen_append_format(out_result->route_source,
                                     sizeof(out_result->route_source),
                                     &route_cursor,
                                     "static int angular_backend_route_%zu(void *context,\n"
                                     "                                  const ng_http_request_t *request,\n"
                                     "                                  ng_http_response_t *response) {\n"
                                     "  const ng_server_binding_t *bindings = g_route_%zu_bindings;\n"
                                     "  const size_t binding_count = %zu;\n"
                                     "  const int status_code = %d;\n"
                                     "  (void)context;\n",
                                     index,
                                     index,
                                     route->binding_count,
                                     route->status_code) != 0) {
      return 1;
    }

    switch (route->response_kind) {
      case NG_RESPONSE_JSON:
        if (server_codegen_append_format(out_result->route_source,
                                         sizeof(out_result->route_source),
                                         &route_cursor,
                                         "  const char *response_expr = \"%s\";\n"
                                         "  json_data *value = NULL;\n"
                                         "  char *text = NULL;\n",
                                         escaped_response_expr) != 0) {
          return 1;
        }
        if (server_codegen_emit_json_response(out_result->route_source,
                                              sizeof(out_result->route_source),
                                              &route_cursor) != 0) {
          return 1;
        }
        break;
      case NG_RESPONSE_TEXT:
        if (server_codegen_append_format(out_result->route_source,
                                         sizeof(out_result->route_source),
                                         &route_cursor,
                                         "  const char *response_expr = \"%s\";\n"
                                         "  json_data *value = NULL;\n"
                                         "  char *text = NULL;\n",
                                         escaped_response_expr) != 0) {
          return 1;
        }
        if (server_codegen_emit_text_response(out_result->route_source,
                                              sizeof(out_result->route_source),
                                              &route_cursor) != 0) {
          return 1;
        }
        break;
      case NG_RESPONSE_RENDER:
        if (server_codegen_append_format(out_result->route_source,
                                         sizeof(out_result->route_source),
                                         &route_cursor,
                                         "  const char *model_expr = \"%s\";\n"
                                         "  const char *template_name = \"%s\";\n"
                                         "  json_data *model = NULL;\n",
                                         escaped_model_expr,
                                         escaped_template) != 0) {
          return 1;
        }
        if (server_codegen_append(out_result->route_source,
                                  sizeof(out_result->route_source),
                                  &route_cursor,
                                  "  response->status_code = status_code;\n"
                                  "  model = angular_server_model_from_expr(model_expr, request, bindings, binding_count);\n"
                                  "  if (ng_server_render_template_response(g_angular_templates,\n"
                                  "                                         g_angular_template_count,\n"
                                  "                                         template_name,\n"
                                  "                                         model,\n"
                                  "                                         request,\n"
                                  "                                         response) != 0) {\n"
                                  "    json_free(model);\n"
                                  "    response->status_code = 500;\n"
                                  "    angular_http_write_error_json(response, \"template render failed\");\n"
                                  "    return 0;\n"
                                  "  }\n"
                                  "  json_free(model);\n"
                                  "  return 0;\n") != 0) {
          return 1;
        }
        break;
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
