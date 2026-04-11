#ifndef ANGULAR_HTTP_SERVICE_H
#define ANGULAR_HTTP_SERVICE_H

#include "helpers/include/runtime/app_runtime.h"
#include "helpers/include/net/http_service.h"

typedef struct {
  ng_runtime_t *runtime;
  ng_http_service_t service;
  ng_http_route_t routes[6];
} angular_http_service_t;

void angular_http_service_init(angular_http_service_t *service,
                               ng_runtime_t *runtime,
                               const char *html_page,
                               const char *css_text,
                               const char *js_text);

#endif
