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

static int server_parse_number_kind(const char *text, ng_local_kind_t *out_kind) {
  size_t index = 0;
  int has_decimal = 0;

  if (text[index] == '-' || text[index] == '+') {
    index += 1;
  }
  if (text[index] == '\0') {
    return 1;
  }

  for (; text[index] != '\0'; ++index) {
    if (text[index] == '.') {
      if (has_decimal) {
        return 1;
      }
      has_decimal = 1;
      continue;
    }
    if (!isdigit((unsigned char)text[index])) {
      return 1;
    }
  }

  *out_kind = has_decimal ? NG_LOCAL_DOUBLE : NG_LOCAL_INT;
  return 0;
}

static int server_parse_object_literal(const char *text,
                                       ng_server_local_ir_t *locals,
                                       size_t *in_out_count,
                                       size_t max_count) {
  const char *cursor = text;

  server_skip_space(&cursor);
  if (*cursor != '{') {
    return 1;
  }
  cursor += 1;

  while (*cursor != '\0') {
    ng_server_local_ir_t *slot;
    char key[64];
    char value[256];
    const char *value_start;
    size_t depth = 0;
    char quote = '\0';

    server_skip_space(&cursor);
    if (*cursor == '}') {
      return 0;
    }
    if (*in_out_count >= max_count) {
      return 1;
    }

    if (server_parse_quoted_string(&cursor, key, sizeof(key)) != 0) {
      const char *ident_start = cursor;
      while (isalnum((unsigned char)*cursor) || *cursor == '_') {
        cursor += 1;
      }
      if (cursor == ident_start) {
        return 1;
      }
      server_trim_copy(key, sizeof(key), ident_start, (size_t)(cursor - ident_start));
    }

    server_skip_space(&cursor);
    if (*cursor != ':') {
      return 1;
    }
    cursor += 1;
    server_skip_space(&cursor);
    value_start = cursor;

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
      if (*cursor == '{' || *cursor == '[' || *cursor == '(') {
        depth += 1;
      } else if ((*cursor == '}' || *cursor == ']' || *cursor == ')') && depth > 0) {
        depth -= 1;
      } else if (depth == 0 && (*cursor == ',' || *cursor == '}')) {
        break;
      }
      cursor += 1;
    }

    slot = &locals[*in_out_count];
    memset(slot, 0, sizeof(*slot));
    snprintf(slot->name, sizeof(slot->name), "%s", key);
    server_trim_copy(value, sizeof(value), value_start, (size_t)(cursor - value_start));

    if ((value[0] == '\'' || value[0] == '"') && value[strlen(value) - 1] == value[0]) {
      size_t length = strlen(value);
      slot->kind = NG_LOCAL_STRING;
      memmove(value, value + 1, length - 2);
      value[length - 2] = '\0';
      snprintf(slot->value, sizeof(slot->value), "%s", value);
    } else if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
      slot->kind = NG_LOCAL_BOOL;
      snprintf(slot->value, sizeof(slot->value), "%s", value);
    } else if (strcmp(value, "null") == 0) {
      slot->kind = NG_LOCAL_NULL;
      slot->value[0] = '\0';
    } else if (server_parse_number_kind(value, &slot->kind) == 0) {
      snprintf(slot->value, sizeof(slot->value), "%s", value);
    } else {
      return 1;
    }

    *in_out_count += 1;
    if (*cursor == ',') {
      cursor += 1;
      continue;
    }
    if (*cursor == '}') {
      return 0;
    }
  }

  return 1;
}

static int server_parse_route_body(const char *body, ng_server_route_ir_t *route) {
  const char *response = strstr(body, "res.");
  const char *cursor;
  const char *paren_start;

  route->status_code = 200;
  if (response == NULL) {
    return 1;
  }

  cursor = response;
  if (strncmp(cursor, "res.status(", 11) == 0) {
    route->status_code = atoi(cursor + 11);
    cursor = strstr(cursor, ").");
    if (cursor == NULL) {
      return 1;
    }
    cursor += 2;
  } else {
    cursor += 4;
  }

  if (strncmp(cursor, "json(", 5) == 0) {
    route->response_kind = NG_RESPONSE_JSON;
    paren_start = cursor + 5;
    return server_parse_object_literal(paren_start, route->locals, &route->local_count, sizeof(route->locals) / sizeof(route->locals[0]));
  }

  if (strncmp(cursor, "send(", 5) == 0) {
    route->response_kind = NG_RESPONSE_TEXT;
    paren_start = cursor + 5;
    if (server_parse_quoted_string(&paren_start, route->response_text, sizeof(route->response_text)) != 0) {
      return 1;
    }
    return 0;
  }

  if (strncmp(cursor, "render(", 7) == 0) {
    route->response_kind = NG_RESPONSE_RENDER;
    paren_start = cursor + 7;
    if (server_parse_quoted_string(&paren_start, route->template_name, sizeof(route->template_name)) != 0) {
      return 1;
    }
    server_skip_space(&paren_start);
    if (*paren_start != ',') {
      return 1;
    }
    paren_start += 1;
    server_skip_space(&paren_start);
    return server_parse_object_literal(paren_start, route->locals, &route->local_count, sizeof(route->locals) / sizeof(route->locals[0]));
  }

  return 1;
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
    if (body_start == NULL || body_start > args_end) {
      return 1;
    }
    body_start += 1;
    if (server_find_matching(body_start, '{', '}', &body_end) != 0) {
      return 1;
    }

    if (server_parse_route_body(body_start, route) != 0) {
      return 1;
    }

    out_set->route_count += 1;
    cursor = body_end + 1;
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

  snprintf(path, sizeof(path), "%s\\app.server.js", server_root);
  if (!output_fs_file_exists(path)) {
    ng_server_route_set_init(out_set);
    return 0;
  }

  return server_parser_parse_file(path, out_set);
}
