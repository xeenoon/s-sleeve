#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ejs_ir.h"
#include "server_ir.h"

static int fail(const char *message) {
  fprintf(stderr, "test_server_backend failed: %s\n", message);
  return 1;
}

int test_server_backend(void) {
  const char *server_source =
      "server.get('/variables-server', (req, res) => {\n"
      "  const dashboard = { title: 'Variables', status: 'Loaded', count: 3, ready: true, metrics: ['A', 'B'] };\n"
      "  res.render('variables', dashboard);\n"
      "});\n"
      "server.post('/api/save', (req, res) => {\n"
      "  const payload = { ok: true, total: 4, mode: body('mode') };\n"
      "  res.status(201).json(payload);\n"
      "});\n";
  const char *template_source =
      "<% layout('layouts/main') %>\n"
      "<h1><%= title %></h1>\n"
      "<% if (ready) { %><p><%- status %></p><% } else { %><p>missing</p><% } %>\n"
      "<ul><% for (metric of metrics) { %><li><%= metric %></li><% } %></ul>\n"
      "<% include('partials/footer') %>\n";
  ng_server_route_set_t *routes;
  ng_ejs_template_t *template_file;
  ng_ejs_template_t *long_template;
  ng_ejs_template_set_t *templates;
  ng_server_codegen_result_t *codegen;
  char long_html[1400];
  size_t fill_index;

  routes = (ng_server_route_set_t *)calloc(1, sizeof(*routes));
  template_file = (ng_ejs_template_t *)calloc(1, sizeof(*template_file));
  long_template = (ng_ejs_template_t *)calloc(1, sizeof(*long_template));
  templates = (ng_ejs_template_set_t *)calloc(1, sizeof(*templates));
  codegen = (ng_server_codegen_result_t *)calloc(1, sizeof(*codegen));
  if (routes == NULL || template_file == NULL || long_template == NULL || templates == NULL || codegen == NULL) {
    free(routes);
    free(template_file);
    free(long_template);
    free(templates);
    free(codegen);
    return fail("allocation failed");
  }

  for (fill_index = 0; fill_index + 1 < sizeof(long_html); ++fill_index) {
    long_html[fill_index] = (char)('a' + (fill_index % 26));
  }
  long_html[sizeof(long_html) - 1] = '\0';

  if (server_parser_parse_source(server_source, routes) != 0) {
    return fail("route parser rejected valid source");
  }
  if (routes->route_count != 2) {
    return fail("unexpected route count");
  }
  if (routes->routes[0].response_kind != NG_RESPONSE_RENDER ||
      strcmp(routes->routes[0].path, "/variables-server") != 0 ||
      strcmp(routes->routes[0].template_name, "variables") != 0 ||
      strcmp(routes->routes[0].model_expr, "dashboard") != 0 ||
      routes->routes[0].binding_count != 1) {
    return fail("render route parse mismatch");
  }
  if (routes->routes[1].response_kind != NG_RESPONSE_JSON ||
      routes->routes[1].status_code != 201 ||
      strcmp(routes->routes[1].response_expr, "payload") != 0 ||
      routes->routes[1].binding_count != 1) {
    return fail("json route parse mismatch");
  }

  if (ejs_parser_parse_text("variables", template_source, template_file) != 0) {
    return fail("ejs parser rejected valid template");
  }
  if (ejs_parser_parse_text("long", long_html, long_template) != 0 || long_template->node_count < 3) {
    return fail("ejs parser failed to split long text into multiple nodes");
  }
  if (strcmp(template_file->layout_name, "layouts/main") != 0 ||
      template_file->node_count < 18 ||
      template_file->nodes[1].kind != NG_EJS_NODE_EXPR ||
      template_file->nodes[3].kind != NG_EJS_NODE_IF_OPEN ||
      template_file->nodes[4].kind != NG_EJS_NODE_TEXT ||
      template_file->nodes[5].kind != NG_EJS_NODE_RAW_EXPR ||
      template_file->nodes[7].kind != NG_EJS_NODE_ELSE ||
      template_file->nodes[9].kind != NG_EJS_NODE_END ||
      template_file->nodes[11].kind != NG_EJS_NODE_FOR_OPEN ||
      template_file->nodes[17].kind != NG_EJS_NODE_INCLUDE) {
    return fail("unexpected ejs node sequence");
  }

  ng_ejs_template_set_init(templates);
  templates->templates[0] = *template_file;
  templates->template_count = 1;
  if (server_codegen_emit(routes, templates, 2, codegen) != 0) {
    return fail("server codegen failed");
  }

  if (strstr(codegen->support_source, "g_angular_templates") == NULL ||
      strstr(codegen->support_source, "g_route_0_bindings") == NULL ||
      strstr(codegen->route_source, "angular_backend_route_0") == NULL ||
      strstr(codegen->route_source, "ng_server_render_template_response") == NULL ||
      strstr(codegen->route_source, "ng_server_eval_expr(response_expr") == NULL ||
      strstr(codegen->route_init, "service->routes[2].path = \"/variables-server\";") == NULL) {
    return fail("generated backend code missing expected output");
  }

  free(routes);
  free(template_file);
  free(long_template);
  free(templates);
  free(codegen);
  return 0;
}
