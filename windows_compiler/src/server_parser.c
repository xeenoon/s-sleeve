#include "server_ir.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_io.h"
#include "output_fs.h"

static void server_skip_space(const char **cursor) {
  while (**cursor == ' ' || **cursor == '\t' || **cursor == '\r' || **cursor == '\n') {
    *cursor += 1;
  }
}

static void server_trim_copy(char *buffer, size_t buffer_size, const char *start, size_t length) {
  size_t begin = 0;
  size_t end = length;

  if (buffer_size == 0) {
    return;
  }

  while (begin < length &&
         (start[begin] == ' ' || start[begin] == '\t' || start[begin] == '\r' || start[begin] == '\n')) {
    begin += 1;
  }
  while (end > begin &&
         (start[end - 1] == ' ' || start[end - 1] == '\t' || start[end - 1] == '\r' || start[end - 1] == '\n')) {
    end -= 1;
  }

  if (end - begin >= buffer_size) {
    end = begin + buffer_size - 1;
  }

  memcpy(buffer, start + begin, end - begin);
  buffer[end - begin] = '\0';
}

static int server_parse_quoted_string(const char **cursor, char *buffer, size_t buffer_size) {
  char quote;
  size_t length = 0;

  server_skip_space(cursor);
  if (**cursor != '\'' && **cursor != '"') {
    return 1;
  }
  quote = **cursor;
  *cursor += 1;

  while (**cursor != '\0' && **cursor != quote) {
    if (**cursor == '\\' && *(*cursor + 1) != '\0') {
      if (length + 1 >= buffer_size) {
        return 1;
      }
      buffer[length++] = *(*cursor + 1);
      *cursor += 2;
      continue;
    }
    if (length + 1 >= buffer_size) {
      return 1;
    }
    buffer[length++] = **cursor;
    *cursor += 1;
  }

  if (**cursor != quote) {
    return 1;
  }
  *cursor += 1;
  buffer[length] = '\0';
  return 0;
}

static int server_find_matching(const char *start, char open, char close, const char **out_end) {
  int depth = 1;
  char quote = '\0';
  const char *cursor = start;

  while (*cursor != '\0') {
    if (quote != '\0') {
      if (*cursor == '\\' && *(cursor + 1) != '\0') {
        cursor += 2;
        continue;
      }
      if (*cursor == quote) {
        quote = '\0';
      }
      cursor += 1;
      continue;
    }

    if (*cursor == '\'' || *cursor == '"') {
      quote = *cursor++;
      continue;
    }
    if (*cursor == open) {
      depth += 1;
    } else if (*cursor == close) {
      depth -= 1;
      if (depth == 0) {
        *out_end = cursor;
        return 0;
      }
    }
    cursor += 1;
  }

  return 1;
}

static int server_find_statement_end(const char *cursor, const char **out_end) {
  size_t depth = 0;
  char quote = '\0';

  while (*cursor != '\0') {
    if (quote != '\0') {
      if (*cursor == '\\' && *(cursor + 1) != '\0') {
        cursor += 2;
        continue;
      }
      if (*cursor == quote) {
        quote = '\0';
      }
      cursor += 1;
      continue;
    }
    if (*cursor == '\'' || *cursor == '"') {
      quote = *cursor++;
      continue;
    }
    if (*cursor == '(' || *cursor == '[' || *cursor == '{') {
      depth += 1;
    } else if ((*cursor == ')' || *cursor == ']' || *cursor == '}') && depth > 0) {
      depth -= 1;
    } else if (*cursor == ';' && depth == 0) {
      *out_end = cursor;
      return 0;
    }
    cursor += 1;
  }
  return 1;
}

static int server_parse_const_statement(const char *statement, ng_server_route_ir_t *route) {
  const char *cursor = statement + 5;
  const char *equals;
  ng_server_binding_ir_t *binding;

  server_skip_space(&cursor);
  if (route->binding_count >= sizeof(route->bindings) / sizeof(route->bindings[0])) {
    return 1;
  }

  equals = strchr(cursor, '=');
  if (equals == NULL) {
    return 1;
  }

  binding = &route->bindings[route->binding_count];
  memset(binding, 0, sizeof(*binding));
  server_trim_copy(binding->name, sizeof(binding->name), cursor, (size_t)(equals - cursor));
  server_trim_copy(binding->expr_source,
                   sizeof(binding->expr_source),
                   equals + 1,
                   strlen(equals + 1));
  route->binding_count += 1;
  return binding->name[0] == '\0' || binding->expr_source[0] == '\0' ? 1 : 0;
}

static int server_parse_response_statement(const char *statement, ng_server_route_ir_t *route) {
  const char *cursor = statement;
  const char *paren_start;
  const char *paren_end;

  route->status_code = 200;
  if (strncmp(cursor, "res.status(", 11) == 0) {
    route->status_code = atoi(cursor + 11);
    cursor = strstr(cursor, ").");
    if (cursor == NULL) {
      return 1;
    }
    cursor += 2;
  } else if (strncmp(cursor, "res.", 4) == 0) {
    cursor += 4;
  } else {
    return 1;
  }

  if (strncmp(cursor, "json(", 5) == 0) {
    route->response_kind = NG_RESPONSE_JSON;
    paren_start = cursor + 5;
    if (server_find_matching(paren_start, '(', ')', &paren_end) != 0) {
      paren_end = strrchr(paren_start, ')');
      if (paren_end == NULL) {
        return 1;
      }
    }
    server_trim_copy(route->response_expr, sizeof(route->response_expr), paren_start, (size_t)(paren_end - paren_start));
    return 0;
  }

  if (strncmp(cursor, "send(", 5) == 0) {
    route->response_kind = NG_RESPONSE_TEXT;
    paren_start = cursor + 5;
    if (server_find_matching(paren_start, '(', ')', &paren_end) != 0) {
      paren_end = strrchr(paren_start, ')');
      if (paren_end == NULL) {
        return 1;
      }
    }
    server_trim_copy(route->response_expr, sizeof(route->response_expr), paren_start, (size_t)(paren_end - paren_start));
    return 0;
  }

  if (strncmp(cursor, "render(", 7) == 0) {
    const char *render_cursor = cursor + 7;
    route->response_kind = NG_RESPONSE_RENDER;
    if (server_parse_quoted_string(&render_cursor, route->template_name, sizeof(route->template_name)) != 0) {
      return 1;
    }
    server_skip_space(&render_cursor);
    if (*render_cursor != ',') {
      return 1;
    }
    render_cursor += 1;
    server_skip_space(&render_cursor);
    paren_end = strrchr(render_cursor, ')');
    if (paren_end == NULL) {
      return 1;
    }
    server_trim_copy(route->model_expr, sizeof(route->model_expr), render_cursor, (size_t)(paren_end - render_cursor));
    return 0;
  }

  return 1;
}

static int server_parse_route_body(const char *body, ng_server_route_ir_t *route) {
  const char *cursor = body;

  route->status_code = 200;
  while (*cursor != '\0') {
    const char *statement_end;
    char statement[2048];

    server_skip_space(&cursor);
    if (*cursor == '\0') {
      break;
    }

    if (server_find_statement_end(cursor, &statement_end) != 0) {
      return 1;
    }
    server_trim_copy(statement, sizeof(statement), cursor, (size_t)(statement_end - cursor));
    if (statement[0] != '\0') {
      if (strncmp(statement, "const ", 6) == 0) {
        if (server_parse_const_statement(statement, route) != 0) {
          return 1;
        }
      } else if (strncmp(statement, "res.", 4) == 0) {
        if (server_parse_response_statement(statement, route) != 0) {
          return 1;
        }
      }
    }
    cursor = statement_end + 1;
  }

  return route->response_kind == NG_RESPONSE_JSON || route->response_kind == NG_RESPONSE_TEXT ||
                 route->response_kind == NG_RESPONSE_RENDER
             ? 0
             : 1;
}

void ng_server_route_set_init(ng_server_route_set_t *set) {
  if (set != NULL) {
    memset(set, 0, sizeof(*set));
  }
}

int server_parser_parse_source(const char *source, ng_server_route_set_t *out_set) {
  const char *cursor = source;

  if (source == NULL || out_set == NULL) {
    return 1;
  }

  ng_server_route_set_init(out_set);

  while (*cursor != '\0') {
    const char *call = strstr(cursor, "server.");
    ng_server_route_ir_t *route;
    const char *args_start;
    const char *args_end;
    const char *body_start;
    const char *body_end;
    const char *route_cursor;
    char body[4096];

    if (call == NULL) {
      break;
    }
    if (out_set->route_count >= sizeof(out_set->routes) / sizeof(out_set->routes[0])) {
      return 1;
    }

    route = &out_set->routes[out_set->route_count];
    memset(route, 0, sizeof(*route));

    if (strncmp(call, "server.get(", 11) == 0) {
      route->method = NG_SERVER_METHOD_GET;
      args_start = call + 11;
    } else if (strncmp(call, "server.post(", 12) == 0) {
      route->method = NG_SERVER_METHOD_POST;
      args_start = call + 12;
    } else {
      cursor = call + 7;
      continue;
    }

    if (server_find_matching(args_start, '(', ')', &args_end) != 0) {
      return 1;
    }

    route_cursor = args_start;
    if (server_parse_quoted_string(&route_cursor, route->path, sizeof(route->path)) != 0) {
      return 1;
    }

    body_start = strchr(route_cursor, '{');
    if (body_start == NULL || body_start >= args_end) {
      return 1;
    }
    if (server_find_matching(body_start + 1, '{', '}', &body_end) != 0) {
      return 1;
    }

    server_trim_copy(body, sizeof(body), body_start + 1, (size_t)(body_end - (body_start + 1)));
    if (server_parse_route_body(body, route) != 0) {
      return 1;
    }

    out_set->route_count += 1;
    cursor = args_end + 1;
  }

  return 0;
}

int server_parser_parse_file(const char *path, ng_server_route_set_t *out_set) {
  file_buffer_t buffer;
  int result;

  if (file_read_all(path, &buffer) != 0) {
    return 1;
  }

  result = server_parser_parse_source(buffer.data, out_set);
  file_buffer_free(&buffer);
  return result;
}

int server_parser_collect(const char *server_root, ng_server_route_set_t *out_set) {
  char path[512];

  if (server_root == NULL || out_set == NULL) {
    return 1;
  }

  ng_server_route_set_init(out_set);
  snprintf(path, sizeof(path), "%s\\app.server.js", server_root);
  if (!output_fs_file_exists(path)) {
    return 0;
  }
  return server_parser_parse_file(path, out_set);
}
