#include "net/http_service.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "data/json.h"

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

static void ng_http_service_write_error(ng_http_response_t *response, int status_code, const char *message) {
  json_data *root;
  char *json_text;

  response->status_code = status_code;
  root = init_json_object();
  if (root == NULL ||
      json_object_add_string(root, "error", message != NULL ? message : "response build failure") != 0) {
    ng_http_response_set_text(response, "{}");
    if (root != NULL) {
      json_free(root);
    }
    return;
  }

  json_text = json_tostring(root);
  json_free(root);
  if (json_text == NULL) {
    ng_http_response_set_text(response, "{}");
    return;
  }

  ng_http_response_set_text(response, json_text);
  free(json_text);
}

void ng_http_service_init(ng_http_service_t *service,
                          const char *html_page,
                          const char *css_text,
                          const char *js_text,
                          const ng_http_route_t *routes,
                          size_t route_count) {
  if (service == NULL) {
    return;
  }

  service->html_page = html_page;
  service->css_text = css_text;
  service->js_text = js_text;
  service->routes = routes;
  service->route_count = route_count;
}

static int ng_http_service_try_static_asset(const ng_http_service_t *service,
                                            const ng_http_request_t *request,
                                            ng_http_response_t *response) {
  if (strcmp(request->method, "GET") != 0) {
    return 0;
  }

  if (strcmp(request->path, "/") == 0) {
    NG_HTTP_TRACE("[service] serving html\n");
    strcpy(response->content_type, "text/html; charset=utf-8");
    if (ng_http_response_set_text(response, service->html_page != NULL ? service->html_page : "") != 0) {
      ng_http_service_write_error(response, 500, "html too large");
    }
    return 1;
  }

  if (strcmp(request->path, "/styles.css") == 0) {
    NG_HTTP_TRACE("[service] serving css\n");
    strcpy(response->content_type, "text/css; charset=utf-8");
    if (ng_http_response_set_text(response, service->css_text != NULL ? service->css_text : "") != 0) {
      ng_http_service_write_error(response, 500, "css too large");
    }
    return 1;
  }

  if (strcmp(request->path, "/app.js") == 0) {
    NG_HTTP_TRACE("[service] serving js\n");
    strcpy(response->content_type, "application/javascript; charset=utf-8");
    if (ng_http_response_set_text(response, service->js_text != NULL ? service->js_text : "") != 0) {
      ng_http_service_write_error(response, 500, "js too large");
    }
    return 1;
  }

  return 0;
}

static int ng_http_service_match_route_path(const char *pattern,
                                            ng_http_request_t *request) {
  const char *pattern_cursor = pattern;
  const char *path_cursor = request->path;

  ng_http_request_clear_route_params(request);

  while (*pattern_cursor != '\0' && *path_cursor != '\0') {
    if (*pattern_cursor == ':') {
      char key[64];
      char value[256];
      size_t key_length = 0;
      size_t value_length = 0;
      pattern_cursor += 1;
      while (*pattern_cursor != '\0' && *pattern_cursor != '/' && key_length + 1 < sizeof(key)) {
        key[key_length++] = *pattern_cursor++;
      }
      key[key_length] = '\0';
      while (*path_cursor != '\0' && *path_cursor != '/' && value_length + 1 < sizeof(value)) {
        value[value_length++] = *path_cursor++;
      }
      value[value_length] = '\0';
      {
        char decoded[256];
        size_t read_index = 0;
        size_t write_index = 0;
        while (value[read_index] != '\0' && write_index + 1 < sizeof(decoded)) {
          if (value[read_index] == '%' &&
              isxdigit((unsigned char)value[read_index + 1]) &&
              isxdigit((unsigned char)value[read_index + 2])) {
            int high = value[read_index + 1] <= '9' ? value[read_index + 1] - '0'
                                                    : 10 + (tolower((unsigned char)value[read_index + 1]) - 'a');
            int low = value[read_index + 2] <= '9' ? value[read_index + 2] - '0'
                                                   : 10 + (tolower((unsigned char)value[read_index + 2]) - 'a');
            decoded[write_index++] = (char)((high << 4) | low);
            read_index += 3;
            continue;
          }
          decoded[write_index++] = value[read_index++] == '+' ? ' ' : value[read_index - 1];
        }
        decoded[write_index] = '\0';
        snprintf(value, sizeof(value), "%s", decoded);
      }
      ng_http_request_set_route_param(request, key, value);
      continue;
    }

    if (*pattern_cursor != *path_cursor) {
      return 0;
    }
    pattern_cursor += 1;
    path_cursor += 1;
  }

  return *pattern_cursor == '\0' && *path_cursor == '\0';
}

int ng_http_service_handle(void *context,
                           const ng_http_request_t *request,
                           ng_http_response_t *response) {
  ng_http_service_t *service = (ng_http_service_t *)context;
  size_t index;
  ng_http_request_t working_request;

  if (service == NULL || request == NULL || response == NULL) {
    return 1;
  }

  if (ng_http_service_try_static_asset(service, request, response)) {
    return 0;
  }

  working_request = *request;
  for (index = 0; index < service->route_count; ++index) {
    const ng_http_route_t *route = &service->routes[index];

    if (route->method == NULL || route->path == NULL || route->handler == NULL) {
      continue;
    }

    working_request = *request;
    if (strcmp(request->method, route->method) == 0 &&
        ng_http_service_match_route_path(route->path, &working_request)) {
      NG_HTTP_TRACE("[service] dispatch %s %s\n", request->method, request->path);
      return route->handler(route->context, &working_request, response);
    }
  }

  if (strcmp(request->method, "POST") == 0) {
    ng_http_service_write_error(response, 404, "unknown post route");
    return 0;
  }

  ng_http_service_write_error(response, 404, "unknown route");
  return 0;
}
