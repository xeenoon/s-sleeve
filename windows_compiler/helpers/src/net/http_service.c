#include "net/http_service.h"

#include <stdio.h>
#include <string.h>

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
  response->status_code = status_code;
  if (ng_http_response_set_format(response, "{\"error\":\"%s\"}", message) != 0) {
    ng_http_response_set_text(response, "{\"error\":\"response build failure\"}");
  }
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

int ng_http_service_handle(void *context,
                           const ng_http_request_t *request,
                           ng_http_response_t *response) {
  ng_http_service_t *service = (ng_http_service_t *)context;
  size_t index;

  if (service == NULL || request == NULL || response == NULL) {
    return 1;
  }

  if (ng_http_service_try_static_asset(service, request, response)) {
    return 0;
  }

  for (index = 0; index < service->route_count; ++index) {
    const ng_http_route_t *route = &service->routes[index];

    if (route->method == NULL || route->path == NULL || route->handler == NULL) {
      continue;
    }

    if (strcmp(request->method, route->method) == 0 && strcmp(request->path, route->path) == 0) {
      NG_HTTP_TRACE("[service] dispatch %s %s\n", request->method, request->path);
      return route->handler(route->context, request, response);
    }
  }

  if (strcmp(request->method, "POST") == 0) {
    ng_http_service_write_error(response, 404, "unknown post route");
    return 0;
  }

  ng_http_service_write_error(response, 404, "unknown route");
  return 0;
}
