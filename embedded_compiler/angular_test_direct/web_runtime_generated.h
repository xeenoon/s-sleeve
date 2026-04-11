#ifndef GENERATED_WEB_RUNTIME_H
#define GENERATED_WEB_RUNTIME_H

#include <Arduino.h>
#include <WebServer.h>

typedef struct {
  const char *method;
  const char *path;
  const char *content_type;
  PGM_P body;
} generated_web_static_route_t;

void generated_web_send_root(WebServer &server);
void generated_web_send_styles(WebServer &server);
void generated_web_send_app_js(WebServer &server);
bool generated_web_try_send_static_route(WebServer &server, const String &path, HTTPMethod method);
size_t generated_web_static_route_count(void);
const generated_web_static_route_t *generated_web_static_route_at(size_t index);

#endif
