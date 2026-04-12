#ifndef ANGULAR_HTTP_SERVICE_H
#define ANGULAR_HTTP_SERVICE_H

#include "helpers/include/runtime/app_runtime.h"
#include "helpers/include/net/http_service.h"

#define ANGULAR_STATIC_ROUTE_COUNT 9
#define ANGULAR_GENERATED_ROUTE_COUNT 18

typedef struct {
  const char *method;
  const char *path;
  const char *content_type;
  const char *body;
} angular_generated_route_t;

typedef struct {
  ng_runtime_t *runtime;
  ng_http_service_t service;
  ng_http_route_t routes[ANGULAR_GENERATED_ROUTE_COUNT > 0 ? ANGULAR_GENERATED_ROUTE_COUNT : 1];
  angular_generated_route_t generated_routes[ANGULAR_STATIC_ROUTE_COUNT > 0 ? ANGULAR_STATIC_ROUTE_COUNT : 1];
} angular_http_service_t;

void angular_http_service_init(angular_http_service_t *service,
                               ng_runtime_t *runtime,
                               const char *html_page,
                               const char *css_text,
                               const char *js_text);

#endif
