#ifndef NG_HTTP_SERVICE_H
#define NG_HTTP_SERVICE_H

#include <stddef.h>

#include "net/http_server.h"

typedef int (*ng_http_route_handler_t)(void *context,
                                       const ng_http_request_t *request,
                                       ng_http_response_t *response);

typedef struct {
  const char *method;
  const char *path;
  ng_http_route_handler_t handler;
  void *context;
} ng_http_route_t;

typedef struct {
  const char *html_page;
  const char *css_text;
  const char *js_text;
  const ng_http_route_t *routes;
  size_t route_count;
} ng_http_service_t;

void ng_http_service_init(ng_http_service_t *service,
                          const char *html_page,
                          const char *css_text,
                          const char *js_text,
                          const ng_http_route_t *routes,
                          size_t route_count);
int ng_http_service_handle(void *context,
                           const ng_http_request_t *request,
                           ng_http_response_t *response);

#endif
