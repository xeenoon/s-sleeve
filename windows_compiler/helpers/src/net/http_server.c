#include "net/http_server.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#ifndef NG_HTTP_TRACE_ENABLED
#define NG_HTTP_TRACE_ENABLED 1
#endif

static const char *ng_http_request_find_pair(const ng_http_pair_t *pairs, size_t count, const char *key);
const char *ng_http_request_header(const ng_http_request_t *request, const char *key);

static int ng_http_hex_value(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'a' && ch <= 'f') {
    return 10 + (ch - 'a');
  }
  if (ch >= 'A' && ch <= 'F') {
    return 10 + (ch - 'A');
  }
  return -1;
}

static void ng_http_decode_component(char *buffer, size_t buffer_size, const char *text, size_t length) {
  size_t cursor = 0;
  size_t index = 0;

  if (buffer_size == 0) {
    return;
  }

  while (index < length && cursor + 1 < buffer_size) {
    if (text[index] == '+' ) {
      buffer[cursor++] = ' ';
      index += 1;
      continue;
    }
    if (text[index] == '%' && index + 2 < length) {
      int high = ng_http_hex_value(text[index + 1]);
      int low = ng_http_hex_value(text[index + 2]);
      if (high >= 0 && low >= 0) {
        buffer[cursor++] = (char)((high << 4) | low);
        index += 3;
        continue;
      }
    }
    buffer[cursor++] = text[index++];
  }

  buffer[cursor] = '\0';
}

#if NG_HTTP_TRACE_ENABLED
#define NG_HTTP_TRACE(...)        \
  do {                            \
    fprintf(stdout, __VA_ARGS__); \
    fflush(stdout);               \
  } while (0)
#else
#define NG_HTTP_TRACE(...) \
  do {                     \
  } while (0)
#endif

static int ng_http_parse_content_length(const char *request_text, size_t *out_length) {
  const char *header = strstr(request_text, "Content-Length:");
  char *end;
  unsigned long parsed;

  if (header == NULL) {
    *out_length = 0;
    return 0;
  }

  header += strlen("Content-Length:");
  while (*header == ' ' || *header == '\t') {
    header += 1;
  }

  parsed = strtoul(header, &end, 10);
  if (header == end) {
    return 1;
  }

  *out_length = (size_t)parsed;
  return 0;
}

static int ng_http_receive_request(SOCKET client_socket, char *buffer, size_t buffer_size) {
  size_t total_received = 0;
  size_t expected_content_length = 0;
  const char *header_end = NULL;

  while (total_received + 1 < buffer_size) {
    int received = recv(client_socket, buffer + total_received, (int)(buffer_size - total_received - 1), 0);
    if (received <= 0) {
      return 1;
    }

    total_received += (size_t)received;
    buffer[total_received] = '\0';

    if (header_end == NULL) {
      header_end = strstr(buffer, "\r\n\r\n");
      if (header_end == NULL) {
        header_end = strstr(buffer, "\n\n");
      }

      if (header_end != NULL && ng_http_parse_content_length(buffer, &expected_content_length) != 0) {
        return 1;
      }
    }

    if (header_end != NULL) {
      size_t header_size = (size_t)(header_end - buffer);
      size_t separator_size = strstr(buffer, "\r\n\r\n") != NULL ? 4u : 2u;
      size_t body_received = total_received - header_size - separator_size;
      if (body_received >= expected_content_length) {
        return 0;
      }
    }
  }

  return 1;
}

static int ng_http_parse_request(const char *request_text, ng_http_request_t *request) {
  const char *header_end;
  const char *body_start;
  const char *line_end;
  const char *headers_start;
  int scanned;
  size_t body_length;

  memset(request, 0, sizeof(*request));
  scanned = sscanf(request_text, "%7s %127s", request->method, request->raw_path);
  if (scanned != 2) {
    return 1;
  }

  {
    const char *query = strchr(request->raw_path, '?');
    if (query != NULL) {
      size_t path_length = (size_t)(query - request->raw_path);
      const char *cursor = query + 1;
      if (path_length >= sizeof(request->path)) {
        path_length = sizeof(request->path) - 1;
      }
      memcpy(request->path, request->raw_path, path_length);
      request->path[path_length] = '\0';

      while (*cursor != '\0' && request->query_count < NG_HTTP_MAX_PAIRS) {
        const char *equals = strchr(cursor, '=');
        const char *amp = strchr(cursor, '&');
        ng_http_pair_t *pair = &request->query_pairs[request->query_count];
        size_t key_length;
        size_t value_length;

        if (equals == NULL) {
          equals = cursor + strlen(cursor);
        }
        if (amp == NULL) {
          amp = cursor + strlen(cursor);
        }
        if (equals > amp) {
          equals = amp;
        }
        key_length = (size_t)(equals - cursor);
        value_length = *equals == '=' ? (size_t)(amp - equals - 1) : 0u;
        if (key_length >= sizeof(pair->key)) {
          key_length = sizeof(pair->key) - 1;
        }
        if (value_length >= sizeof(pair->value)) {
          value_length = sizeof(pair->value) - 1;
        }
        ng_http_decode_component(pair->key, sizeof(pair->key), cursor, key_length);
        if (*equals == '=') {
          ng_http_decode_component(pair->value, sizeof(pair->value), equals + 1, value_length);
        } else {
          pair->value[0] = '\0';
        }
        request->query_count += 1;
        cursor = *amp == '&' ? amp + 1 : amp;
        if (*cursor == '\0') {
          break;
        }
      }
    } else {
      snprintf(request->path, sizeof(request->path), "%s", request->raw_path);
    }
  }

  line_end = strstr(request_text, "\r\n");
  if (line_end == NULL) {
    line_end = strstr(request_text, "\n");
  }
  headers_start = line_end != NULL ? line_end + (line_end[0] == '\r' ? 2 : 1) : request_text;

  header_end = strstr(request_text, "\r\n\r\n");
  if (header_end == NULL) {
    header_end = strstr(request_text, "\n\n");
    if (header_end == NULL) {
      request->body[0] = '\0';
      request->body_length = 0;
      return 0;
    }
    body_start = header_end + 2;
  } else {
    body_start = header_end + 4;
  }

  body_length = strlen(body_start);
  if (body_length >= sizeof(request->body)) {
    body_length = sizeof(request->body) - 1;
  }

  memcpy(request->body, body_start, body_length);
  request->body[body_length] = '\0';
  request->body_length = body_length;

  {
    const char *cursor = headers_start;
    while (cursor < header_end && request->header_count < NG_HTTP_MAX_PAIRS) {
      const char *line_break = strstr(cursor, "\r\n");
      const char *colon = strchr(cursor, ':');
      ng_http_pair_t *pair = &request->headers[request->header_count];
      size_t key_length;
      size_t value_length;
      const char *value_start;

      if (line_break == NULL || line_break > header_end) {
        line_break = strchr(cursor, '\n');
        if (line_break == NULL || line_break > header_end) {
          line_break = header_end;
        }
      }
      if (colon == NULL || colon > line_break) {
        cursor = line_break + ((line_break[0] == '\r' && line_break[1] == '\n') ? 2 : 1);
        continue;
      }
      key_length = (size_t)(colon - cursor);
      value_start = colon + 1;
      while (*value_start == ' ' || *value_start == '\t') {
        value_start += 1;
      }
      value_length = (size_t)(line_break - value_start);
      if (key_length >= sizeof(pair->key)) {
        key_length = sizeof(pair->key) - 1;
      }
      if (value_length >= sizeof(pair->value)) {
        value_length = sizeof(pair->value) - 1;
      }
      ng_http_decode_component(pair->key, sizeof(pair->key), cursor, key_length);
      ng_http_decode_component(pair->value, sizeof(pair->value), value_start, value_length);
      request->header_count += 1;
      cursor = line_break + ((line_break[0] == '\r' && line_break[1] == '\n') ? 2 : 1);
    }
  }

  snprintf(request->content_type, sizeof(request->content_type), "%s", ng_http_request_header(request, "Content-Type"));

  if (strncmp(request->content_type, "application/json", 16) == 0) {
    const char *cursor = request->body;
    while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') {
      cursor += 1;
    }
    if (*cursor == '{') {
      cursor += 1;
      while (*cursor != '\0' && request->body_count < NG_HTTP_MAX_PAIRS) {
        ng_http_pair_t *pair = &request->body_pairs[request->body_count];
        const char *key_start;
        const char *key_end;
        const char *value_start;
        const char *value_end;
        size_t key_length;
        size_t value_length;

        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n' || *cursor == ',') {
          cursor += 1;
        }
        if (*cursor == '}') {
          break;
        }
        if (*cursor != '"' && *cursor != '\'') {
          break;
        }
        key_start = ++cursor;
        while (*cursor != '\0' && *cursor != '"' && *cursor != '\'') {
          cursor += 1;
        }
        key_end = cursor;
        if (*cursor == '\0') {
          break;
        }
        cursor += 1;
        while (*cursor == ' ' || *cursor == '\t' || *cursor == ':') {
          cursor += 1;
        }
        value_start = cursor;
        if (*cursor == '"' || *cursor == '\'') {
          char quote = *cursor++;
          value_start = cursor;
          while (*cursor != '\0' && *cursor != quote) {
            cursor += 1;
          }
          value_end = cursor;
          if (*cursor == quote) {
            cursor += 1;
          }
        } else {
          while (*cursor != '\0' && *cursor != ',' && *cursor != '}') {
            cursor += 1;
          }
          value_end = cursor;
          while (value_end > value_start && isspace((unsigned char)value_end[-1])) {
            value_end -= 1;
          }
        }
        key_length = (size_t)(key_end - key_start);
        value_length = (size_t)(value_end - value_start);
        if (key_length >= sizeof(pair->key)) {
          key_length = sizeof(pair->key) - 1;
        }
        if (value_length >= sizeof(pair->value)) {
          value_length = sizeof(pair->value) - 1;
        }
        memcpy(pair->key, key_start, key_length);
        pair->key[key_length] = '\0';
        memcpy(pair->value, value_start, value_length);
        pair->value[value_length] = '\0';
        request->body_count += 1;
      }
    }
  } else if (strncmp(request->content_type, "application/x-www-form-urlencoded", 33) == 0) {
    const char *cursor = request->body;
    while (*cursor != '\0' && request->body_count < NG_HTTP_MAX_PAIRS) {
      const char *equals = strchr(cursor, '=');
      const char *amp = strchr(cursor, '&');
      ng_http_pair_t *pair = &request->body_pairs[request->body_count];
      size_t key_length;
      size_t value_length;

      if (equals == NULL) {
        equals = cursor + strlen(cursor);
      }
      if (amp == NULL) {
        amp = cursor + strlen(cursor);
      }
      if (equals > amp) {
        equals = amp;
      }
      key_length = (size_t)(equals - cursor);
      value_length = *equals == '=' ? (size_t)(amp - equals - 1) : 0u;
      if (key_length >= sizeof(pair->key)) {
        key_length = sizeof(pair->key) - 1;
      }
      if (value_length >= sizeof(pair->value)) {
        value_length = sizeof(pair->value) - 1;
      }
      ng_http_decode_component(pair->key, sizeof(pair->key), cursor, key_length);
      if (*equals == '=') {
        ng_http_decode_component(pair->value, sizeof(pair->value), equals + 1, value_length);
      } else {
        pair->value[0] = '\0';
      }
      request->body_count += 1;
      cursor = *amp == '&' ? amp + 1 : amp;
    }
  }

  return 0;
}

static const char *ng_http_request_find_pair(const ng_http_pair_t *pairs, size_t count, const char *key) {
  size_t index;
  for (index = 0; index < count; ++index) {
    if (_stricmp(pairs[index].key, key) == 0) {
      return pairs[index].value;
    }
  }
  return "";
}

static const char *ng_http_status_text(int status_code) {
  switch (status_code) {
    case 200:
      return "OK";
    case 400:
      return "Bad Request";
    case 404:
      return "Not Found";
    case 405:
      return "Method Not Allowed";
    case 500:
      return "Internal Server Error";
    default:
      return "OK";
  }
}

void ng_http_response_init(ng_http_response_t *response) {
  if (response == NULL) {
    return;
  }
  response->status_code = 200;
  memset(response->content_type, 0, sizeof(response->content_type));
  strcpy(response->content_type, "application/json");
  response->body[0] = '\0';
  response->dynamic_body = NULL;
  response->dynamic_body_length = 0;
  if (ng_http_response_set_text(response, "{}") != 0) {
    response->body[0] = '{';
    response->body[1] = '}';
    response->body[2] = '\0';
  }
}

void ng_http_response_dispose(ng_http_response_t *response) {
  if (response == NULL) {
    return;
  }
  if (response->dynamic_body != NULL) {
    free(response->dynamic_body);
    response->dynamic_body = NULL;
  }
  response->dynamic_body_length = 0;
  response->body[0] = '\0';
}

const char *ng_http_response_body(const ng_http_response_t *response) {
  if (response == NULL) {
    return "";
  }
  if (response->dynamic_body != NULL) {
    return response->dynamic_body;
  }
  return response->body;
}

int ng_http_response_set_text(ng_http_response_t *response, const char *text) {
  size_t length;
  char *dynamic_copy;

  if (response == NULL) {
    return 1;
  }

  if (text == NULL) {
    text = "";
  }

  length = strlen(text);
  if (response->dynamic_body != NULL) {
    free(response->dynamic_body);
    response->dynamic_body = NULL;
    response->dynamic_body_length = 0;
  }

  if (length < sizeof(response->body)) {
    memcpy(response->body, text, length + 1);
    response->dynamic_body_length = length;
    return 0;
  }

  dynamic_copy = (char *)malloc(length + 1);
  if (dynamic_copy == NULL) {
    return 1;
  }

  memcpy(dynamic_copy, text, length + 1);
  response->dynamic_body = dynamic_copy;
  response->dynamic_body_length = length;
  response->body[0] = '\0';
  return 0;
}

int ng_http_response_set_format(ng_http_response_t *response, const char *format, ...) {
  int needed;
  va_list args;
  char *dynamic_text;

  if (response == NULL || format == NULL) {
    return 1;
  }

  va_start(args, format);
  needed = _vscprintf(format, args);
  va_end(args);
  if (needed < 0) {
    return 1;
  }

  if ((size_t)needed < sizeof(response->body)) {
    va_start(args, format);
    vsnprintf(response->body, sizeof(response->body), format, args);
    va_end(args);
    if (response->dynamic_body != NULL) {
      free(response->dynamic_body);
      response->dynamic_body = NULL;
    }
    response->dynamic_body_length = (size_t)needed;
    return 0;
  }

  dynamic_text = (char *)malloc((size_t)needed + 1u);
  if (dynamic_text == NULL) {
    return 1;
  }

  va_start(args, format);
  vsnprintf(dynamic_text, (size_t)needed + 1u, format, args);
  va_end(args);

  if (response->dynamic_body != NULL) {
    free(response->dynamic_body);
  }
  response->dynamic_body = dynamic_text;
  response->dynamic_body_length = (size_t)needed;
  response->body[0] = '\0';
  return 0;
}

const char *ng_http_request_query(const ng_http_request_t *request, const char *key) {
  if (request == NULL || key == NULL) {
    return "";
  }
  return ng_http_request_find_pair(request->query_pairs, request->query_count, key);
}

const char *ng_http_request_body_field(const ng_http_request_t *request, const char *key) {
  if (request == NULL || key == NULL) {
    return "";
  }
  return ng_http_request_find_pair(request->body_pairs, request->body_count, key);
}

const char *ng_http_request_route_param(const ng_http_request_t *request, const char *key) {
  if (request == NULL || key == NULL) {
    return "";
  }
  return ng_http_request_find_pair(request->route_params, request->route_param_count, key);
}

const char *ng_http_request_header(const ng_http_request_t *request, const char *key) {
  if (request == NULL || key == NULL) {
    return "";
  }
  return ng_http_request_find_pair(request->headers, request->header_count, key);
}

void ng_http_request_set_route_param(ng_http_request_t *request, const char *key, const char *value) {
  ng_http_pair_t *pair;
  if (request == NULL || key == NULL || value == NULL || request->route_param_count >= NG_HTTP_MAX_PAIRS) {
    return;
  }
  pair = &request->route_params[request->route_param_count++];
  snprintf(pair->key, sizeof(pair->key), "%s", key);
  snprintf(pair->value, sizeof(pair->value), "%s", value);
}

void ng_http_request_clear_route_params(ng_http_request_t *request) {
  if (request != NULL) {
    request->route_param_count = 0;
  }
}

static int ng_http_send_response(SOCKET client_socket, const ng_http_response_t *response) {
  char packet[131072];
  const char *body_text = ng_http_response_body(response);
  int body_length = (int)strlen(body_text);
  int packet_length;

  packet_length = snprintf(packet,
                           sizeof(packet),
                           "HTTP/1.1 %d %s\r\n"
                           "Content-Type: %s\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           response->status_code,
                           ng_http_status_text(response->status_code),
                           response->content_type,
                           body_length,
                           body_text);
  if (packet_length < 0 || packet_length >= (int)sizeof(packet)) {
    return 1;
  }

  if (send(client_socket, packet, packet_length, 0) != packet_length) {
    return 1;
  }

  shutdown(client_socket, SD_BOTH);
  return 0;
}

int ng_http_server_serve(unsigned short port,
                         ng_http_handler_t handler,
                         void *context,
                         int max_requests) {
  WSADATA wsa_data;
  SOCKET server_socket = INVALID_SOCKET;
  int requests_handled = 0;
  int reuse_address = 1;
  int dual_stack = 0;
  struct sockaddr_in6 server_address;

  if (handler == NULL) {
    return 1;
  }

  NG_HTTP_TRACE("[http] listen 127.0.0.1:%u max_requests=%d\n", port, max_requests);

  if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
    return 1;
  }

  server_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
  if (server_socket == INVALID_SOCKET) {
    WSACleanup();
    return 1;
  }

  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse_address, sizeof(reuse_address));
  setsockopt(server_socket, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&dual_stack, sizeof(dual_stack));

  memset(&server_address, 0, sizeof(server_address));
  server_address.sin6_family = AF_INET6;
  server_address.sin6_addr = in6addr_any;
  server_address.sin6_port = htons(port);

  if (bind(server_socket, (SOCKADDR *)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
    closesocket(server_socket);
    WSACleanup();
    return 1;
  }

  if (listen(server_socket, 4) == SOCKET_ERROR) {
    closesocket(server_socket);
    WSACleanup();
    return 1;
  }

  while (max_requests <= 0 || requests_handled < max_requests) {
    SOCKET client_socket;
    char request_buffer[16384];
    ng_http_request_t request;
    ng_http_response_t response;

    client_socket = accept(server_socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET) {
      NG_HTTP_TRACE("[http] accept failed\n");
      closesocket(server_socket);
      WSACleanup();
      return 1;
    }

    if (ng_http_receive_request(client_socket, request_buffer, sizeof(request_buffer)) != 0) {
      NG_HTTP_TRACE("[http] receive failed request_index=%d\n", requests_handled + 1);
      closesocket(client_socket);
      continue;
    }

    ng_http_response_init(&response);
    if (ng_http_parse_request(request_buffer, &request) != 0) {
      response.status_code = 400;
      strcpy(response.body, "{\"error\":\"invalid request\"}");
      NG_HTTP_TRACE("[http] parse failed request_index=%d\n", requests_handled + 1);
    } else if (handler(context, &request, &response) != 0) {
      response.status_code = 500;
      strcpy(response.body, "{\"error\":\"handler failure\"}");
      NG_HTTP_TRACE("[http] handler failed method=%s path=%s request_index=%d\n",
                    request.method,
                    request.path,
                    requests_handled + 1);
    } else {
      NG_HTTP_TRACE("[http] %s %s -> %d %s request_index=%d\n",
                    request.method,
                    request.path,
                    response.status_code,
                    response.content_type,
                    requests_handled + 1);
    }

    ng_http_send_response(client_socket, &response);
    ng_http_response_dispose(&response);
    closesocket(client_socket);
    requests_handled += 1;
  }

  NG_HTTP_TRACE("[http] server exiting after requests_handled=%d\n", requests_handled);
  closesocket(server_socket);
  WSACleanup();
  return 0;
}
