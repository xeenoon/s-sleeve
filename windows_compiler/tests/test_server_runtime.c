#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data/json.h"
#include "net/http_server.h"
#include "runtime/server_runtime.h"

static int fail(const char *message) {
  fprintf(stderr, "test_server_runtime failed: %s\n", message);
  return 1;
}

int test_server_runtime(void) {
  ng_http_request_t request;
  ng_http_response_t response;
  ng_server_binding_t bindings[2];
  json_data *value;
  json_data *model;
  char long_note[900];
  size_t long_index;
  static const ng_template_node_t layout_nodes[] = {
      {NG_TEMPLATE_NODE_TEXT, "<html><body>"},
      {NG_TEMPLATE_NODE_INCLUDE, "partials/nav|nav"},
      {NG_TEMPLATE_NODE_RAW_EXPR, "body"},
      {NG_TEMPLATE_NODE_END, ""},
      {NG_TEMPLATE_NODE_TEXT, "</body></html>"}};
  static const ng_template_node_t nav_nodes[] = {
      {NG_TEMPLATE_NODE_TEXT, "<nav>"},
      {NG_TEMPLATE_NODE_FOR_OPEN, "link|links"},
      {NG_TEMPLATE_NODE_TEXT, "<a>"},
      {NG_TEMPLATE_NODE_EXPR, "link.label"},
      {NG_TEMPLATE_NODE_TEXT, "</a>"},
      {NG_TEMPLATE_NODE_END, ""},
      {NG_TEMPLATE_NODE_TEXT, "</nav>"}};
  static const ng_template_node_t page_nodes[] = {
      {NG_TEMPLATE_NODE_TEXT, "<section><h1>"},
      {NG_TEMPLATE_NODE_EXPR, "title"},
      {NG_TEMPLATE_NODE_TEXT, "</h1>"},
      {NG_TEMPLATE_NODE_IF_OPEN, "show"},
      {NG_TEMPLATE_NODE_TEXT, "<p>"},
      {NG_TEMPLATE_NODE_EXPR, "subtitle"},
      {NG_TEMPLATE_NODE_TEXT, "</p>"},
      {NG_TEMPLATE_NODE_ELSE, ""},
      {NG_TEMPLATE_NODE_TEXT, "<p>hidden</p>"},
      {NG_TEMPLATE_NODE_END, ""},
      {NG_TEMPLATE_NODE_TEXT, "<ul>"},
      {NG_TEMPLATE_NODE_FOR_OPEN, "item|items"},
      {NG_TEMPLATE_NODE_TEXT, "<li>"},
      {NG_TEMPLATE_NODE_EXPR, "item"},
      {NG_TEMPLATE_NODE_TEXT, "</li>"},
      {NG_TEMPLATE_NODE_END, ""},
      {NG_TEMPLATE_NODE_TEXT, "<div>"},
      {NG_TEMPLATE_NODE_EXPR, "longNote"},
      {NG_TEMPLATE_NODE_TEXT, "</div>"},
      {NG_TEMPLATE_NODE_TEXT, "</ul></section>"}};
  static const ng_template_def_t templates[] = {
      {"layouts/main", "", layout_nodes, sizeof(layout_nodes) / sizeof(layout_nodes[0])},
      {"partials/nav", "", nav_nodes, sizeof(nav_nodes) / sizeof(nav_nodes[0])},
      {"pages/demo", "layouts/main", page_nodes, sizeof(page_nodes) / sizeof(page_nodes[0])}};

  memset(&request, 0, sizeof(request));
  memset(&response, 0, sizeof(response));
  snprintf(request.method, sizeof(request.method), "GET");
  snprintf(request.path, sizeof(request.path), "/reports/daily-brief");
  snprintf(request.query_pairs[0].key, sizeof(request.query_pairs[0].key), "audience");
  snprintf(request.query_pairs[0].value, sizeof(request.query_pairs[0].value), "clinical lead");
  request.query_count = 1;
  snprintf(request.body_pairs[0].key, sizeof(request.body_pairs[0].key), "mode");
  snprintf(request.body_pairs[0].value, sizeof(request.body_pairs[0].value), "guided");
  request.body_count = 1;
  snprintf(request.headers[0].key, sizeof(request.headers[0].key), "X-Operator");
  snprintf(request.headers[0].value, sizeof(request.headers[0].value), "ops-desk");
  request.header_count = 1;
  ng_http_request_set_route_param(&request, "reportId", "daily-brief");

  bindings[0].name = "navModel";
  bindings[0].expr_source = "{ nav: { links: [ { label: 'Home' }, { label: 'Reports' } ] } }";
  bindings[1].name = "payload";
  bindings[1].expr_source = "{ title: upper(param('reportId')), subtitle: title(query('audience')), show: true, items: ['alpha', body('mode')], nav: navModel.nav }";

  for (long_index = 0; long_index + 1 < sizeof(long_note); ++long_index) {
    long_note[long_index] = (char)('a' + (long_index % 26));
  }
  long_note[sizeof(long_note) - 1] = '\0';

  value = ng_server_eval_expr("payload", &request, bindings, 2, NULL, NULL);
  if (value == NULL || value->type != JSON_OBJECT) {
    return fail("expression evaluation did not produce object model");
  }
  if (strcmp(get_value(value, "title")->as.string.data, "DAILY-BRIEF") != 0 ||
      strcmp(get_value(value, "subtitle")->as.string.data, "Clinical Lead") != 0) {
    json_free(value);
    return fail("expression helpers or request access failed");
  }

  model = value;
  json_object_add_string(model, "longNote", long_note);
  if (ng_server_render_template_response(templates,
                                         sizeof(templates) / sizeof(templates[0]),
                                         "pages/demo",
                                         model,
                                         &request,
                                         &response) != 0) {
    json_free(model);
    return fail("template render failed");
  }

  if (response.status_code != 0 && response.status_code != 200) {
    json_free(model);
    return fail("unexpected response status");
  }
  if (strstr(response.body, "<nav><a>Home</a><a>Reports</a></nav>") == NULL ||
      strstr(response.body, "<h1>DAILY-BRIEF</h1>") == NULL ||
      strstr(response.body, "<p>Clinical Lead</p>") == NULL ||
      strstr(response.body, "<li>guided</li>") == NULL ||
      strstr(response.body, "uvwxyzabcdef") == NULL) {
    json_free(model);
    return fail("rendered response missing expected layout/include/loop output");
  }

  json_free(model);
  return 0;
}
