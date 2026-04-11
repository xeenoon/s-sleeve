#include <Arduino.h>
#include <WebServer.h>

#include <string.h>

#include "generated/web_runtime_generated.h"
#include "web_page.h"

{{ROUTE_BODIES}}

static const generated_web_static_route_t g_generated_routes[] = {
{{ROUTE_TABLE}}};

static const char *generated_web_method_name(HTTPMethod method) {
  switch (method) {
    case HTTP_GET: return "GET";
    case HTTP_POST: return "POST";
    case HTTP_PUT: return "PUT";
    case HTTP_DELETE: return "DELETE";
    case HTTP_PATCH: return "PATCH";
    case HTTP_OPTIONS: return "OPTIONS";
    default: return "GET";
  }
}

void generated_web_send_root(WebServer &server) {
  server.send_P(200, "text/html", INDEX_HTML);
}

void generated_web_send_styles(WebServer &server) {
  server.send_P(200, "text/css", STYLES_CSS);
}

void generated_web_send_app_js(WebServer &server) {
  server.send_P(200, "application/javascript", APP_JS);
}

size_t generated_web_static_route_count(void) {
  return sizeof(g_generated_routes) / sizeof(g_generated_routes[0]);
}

const generated_web_static_route_t *generated_web_static_route_at(size_t index) {
  if (index >= generated_web_static_route_count()) {
    return nullptr;
  }
  return &g_generated_routes[index];
}

bool generated_web_try_send_static_route(WebServer &server, const String &path, HTTPMethod method) {
  const char *method_name = generated_web_method_name(method);
  size_t index;
  for (index = 0; index < generated_web_static_route_count(); ++index) {
    const generated_web_static_route_t &route = g_generated_routes[index];
    if (strcmp(route.method, method_name) == 0 && path.equals(route.path)) {
      server.send_P(200, route.content_type, route.body);
      return true;
    }
  }
  return false;
}
