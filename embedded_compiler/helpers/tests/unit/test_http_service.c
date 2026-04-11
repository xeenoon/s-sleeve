#include "net/http_service.h"
#include "data/json.h"
#include "support/test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int value;
} test_http_context_t;

static int test_state_handler(void *context, const ng_http_request_t *request, ng_http_response_t *response) {
  test_http_context_t *test_context = (test_http_context_t *)context;
  json_data *root;
  char *json_text;
  (void)request;
  root = init_json_object();
  if (root == NULL || json_object_add_number(root, "value", test_context->value) != 0) {
    if (root != NULL) {
      json_free(root);
    }
    return 1;
  }
  json_text = json_tostring(root);
  json_free(root);
  if (json_text == NULL) {
    return 1;
  }
  ng_http_response_set_text(response, json_text);
  free(json_text);
  return 0;
}

static int test_post_handler(void *context, const ng_http_request_t *request, ng_http_response_t *response) {
  test_http_context_t *test_context = (test_http_context_t *)context;
  json_data *root;
  char *json_text;
  if (strstr(request->body, "42") != NULL) {
    test_context->value = 42;
  }
  root = init_json_object();
  if (root == NULL || json_object_add_number(root, "value", test_context->value) != 0) {
    if (root != NULL) {
      json_free(root);
    }
    return 1;
  }
  json_text = json_tostring(root);
  json_free(root);
  if (json_text == NULL) {
    return 1;
  }
  ng_http_response_set_text(response, json_text);
  free(json_text);
  return 0;
}

void test_http_service(ng_test_context_t *context) {
  test_http_context_t test_context = {7};
  ng_http_route_t routes[2];
  ng_http_service_t service;
  ng_http_request_t request;
  ng_http_response_t response;
  char large_js[8192];
  size_t fill_index;

  for (fill_index = 0; fill_index + 1 < sizeof(large_js); ++fill_index) {
    large_js[fill_index] = (char)('a' + (fill_index % 26u));
  }
  large_js[sizeof(large_js) - 1] = '\0';

  routes[0].method = "GET";
  routes[0].path = "/state";
  routes[0].handler = test_state_handler;
  routes[0].context = &test_context;
  routes[1].method = "POST";
  routes[1].path = "/reading";
  routes[1].handler = test_post_handler;
  routes[1].context = &test_context;

  ng_http_service_init(&service,
                       "<!doctype html><div id=\"reading\"></div>",
                       ".limb{stroke:#d66b2d;}.joint{fill:#241a14;}",
                       large_js,
                       routes,
                       2);

  memset(&request, 0, sizeof(request));
  strcpy(request.method, "GET");
  strcpy(request.path, "/");
  ng_http_response_init(&response);
  NG_ASSERT_INT_EQ(context, 0, ng_http_service_handle(&service, &request, &response));
  NG_ASSERT_STR_EQ(context, "text/html; charset=utf-8", response.content_type);
  NG_ASSERT_TRUE(context, strstr(ng_http_response_body(&response), "id=\"reading\"") != NULL);
  ng_http_response_dispose(&response);

  memset(&request, 0, sizeof(request));
  strcpy(request.method, "GET");
  strcpy(request.path, "/styles.css");
  ng_http_response_init(&response);
  NG_ASSERT_INT_EQ(context, 0, ng_http_service_handle(&service, &request, &response));
  NG_ASSERT_STR_EQ(context, "text/css; charset=utf-8", response.content_type);
  NG_ASSERT_TRUE(context, strstr(ng_http_response_body(&response), ".joint") != NULL);
  ng_http_response_dispose(&response);

  memset(&request, 0, sizeof(request));
  strcpy(request.method, "GET");
  strcpy(request.path, "/app.js");
  ng_http_response_init(&response);
  NG_ASSERT_INT_EQ(context, 0, ng_http_service_handle(&service, &request, &response));
  NG_ASSERT_STR_EQ(context, "application/javascript; charset=utf-8", response.content_type);
  NG_ASSERT_INT_EQ(context, (int)strlen(large_js), (int)strlen(ng_http_response_body(&response)));
  NG_ASSERT_TRUE(context, strcmp(ng_http_response_body(&response), large_js) == 0);
  ng_http_response_dispose(&response);

  memset(&request, 0, sizeof(request));
  strcpy(request.method, "GET");
  strcpy(request.path, "/state");
  ng_http_response_init(&response);
  NG_ASSERT_INT_EQ(context, 0, ng_http_service_handle(&service, &request, &response));
  NG_ASSERT_TRUE(context, strstr(ng_http_response_body(&response), "\"value\":7") != NULL);
  ng_http_response_dispose(&response);

  memset(&request, 0, sizeof(request));
  strcpy(request.method, "POST");
  strcpy(request.path, "/reading");
  strcpy(request.body, "{\"reading\":42}");
  request.body_length = strlen(request.body);
  ng_http_response_init(&response);
  NG_ASSERT_INT_EQ(context, 0, ng_http_service_handle(&service, &request, &response));
  NG_ASSERT_TRUE(context, strstr(ng_http_response_body(&response), "\"value\":42") != NULL);
  ng_http_response_dispose(&response);

  memset(&request, 0, sizeof(request));
  strcpy(request.method, "GET");
  strcpy(request.path, "/missing");
  ng_http_response_init(&response);
  NG_ASSERT_INT_EQ(context, 0, ng_http_service_handle(&service, &request, &response));
  NG_ASSERT_INT_EQ(context, 404, response.status_code);
  NG_ASSERT_TRUE(context, strstr(ng_http_response_body(&response), "unknown route") != NULL);
  ng_http_response_dispose(&response);
}
