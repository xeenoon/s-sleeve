#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data/json.h"
#include "data/json_utils.h"
#include "net/http_service.h"
#include "net/http_server.h"
#include "runtime/app_runtime.h"

typedef struct {
  ng_runtime_t *runtime;
  ng_http_service_t service;
  ng_http_route_t routes[2];
} helper_server_t;

static int helper_state_handler(void *context, const ng_http_request_t *request, ng_http_response_t *response) {
  helper_server_t *server = (helper_server_t *)context;
  const char *keys[] = {"reading"};
  char *json_text;
  (void)request;

  json_text = ng_build_runtime_json(server->runtime, keys, sizeof(keys) / sizeof(keys[0]));
  if (json_text == NULL) {
    json_data *error_root = init_json_object();
    char *error_text = NULL;
    response->status_code = 500;
    if (error_root != NULL) {
      json_object_add_string(error_root, "error", "json build failed");
      error_text = json_tostring(error_root);
      json_free(error_root);
    }
    ng_http_response_set_text(response, error_text != NULL ? error_text : "{}");
    free(error_text);
    return 0;
  }

  ng_http_response_set_text(response, json_text);
  free(json_text);
  return 0;
}

static int helper_value_handler(void *context, const ng_http_request_t *request, ng_http_response_t *response) {
  helper_server_t *server = (helper_server_t *)context;
  (void)request;
  strcpy(response->content_type, "text/plain");
  ng_http_response_set_format(response, "%d", ng_runtime_get_int(server->runtime, "reading", 0));
  return 0;
}

int main(int argc, char **argv) {
  unsigned short port = 18080;
  int max_requests = 2;
  ng_runtime_t runtime;
  helper_server_t server;

  if (argc >= 2) {
    port = (unsigned short)atoi(argv[1]);
  }
  if (argc >= 3) {
    max_requests = atoi(argv[2]);
  }

  ng_runtime_init(&runtime);
  ng_runtime_set_int(&runtime, "reading", 3100);
  server.runtime = &runtime;
  server.routes[0].method = "GET";
  server.routes[0].path = "/state";
  server.routes[0].handler = helper_state_handler;
  server.routes[0].context = &server;
  server.routes[1].method = "GET";
  server.routes[1].path = "/value";
  server.routes[1].handler = helper_value_handler;
  server.routes[1].context = &server;
  ng_http_service_init(&server.service, "<html></html>", "body{}", "fetch('/state');", server.routes, 2);

  printf("helper_server listening on 127.0.0.1:%u for %d request(s)\n", port, max_requests);
  fflush(stdout);
  return ng_http_server_serve(port, ng_http_service_handle, &server.service, max_requests);
}
