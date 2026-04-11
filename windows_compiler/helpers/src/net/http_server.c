#include "net/http_server.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifndef NG_HTTP_TRACE_ENABLED
#define NG_HTTP_TRACE_ENABLED 1
#endif

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
  int scanned;
  size_t body_length;

  memset(request, 0, sizeof(*request));
  scanned = sscanf(request_text, "%7s %127s", request->method, request->path);
  if (scanned != 2) {
    return 1;
  }

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
  return 0;
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
