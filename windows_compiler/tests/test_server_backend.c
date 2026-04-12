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
      "  res.render('variables', { title: 'Variables', status: 'Loaded', count: 3, ready: true });\n"
      "});\n"
      "server.post('/api/save', (req, res) => {\n"
      "  res.status(201).json({ ok: true, total: 4 });\n"
      "});\n";
  const char *template_source =
      "<!DOCTYPE html>\n"
      "<html><body><h1><%= title %></h1><% if (ready) { %><p><%- status %></p><% } %></body></html>\n";
  ng_server_route_set_t *routes;
  ng_ejs_template_t *template_file;
  ng_ejs_template_set_t *templates;
  ng_server_codegen_result_t *codegen;

  routes = (ng_server_route_set_t *)calloc(1, sizeof(*routes));
  template_file = (ng_ejs_template_t *)calloc(1, sizeof(*template_file));
  templates = (ng_ejs_template_set_t *)calloc(1, sizeof(*templates));
  codegen = (ng_server_codegen_result_t *)calloc(1, sizeof(*codegen));
  if (routes == NULL || template_file == NULL || templates == NULL || codegen == NULL) {
    free(routes);
    free(template_file);
    free(templates);
    free(codegen);
    return fail("allocation failed");
  }

  if (server_parser_parse_source(server_source, routes) != 0) {
    return fail("route parser rejected valid source");
  }
  if (routes->route_count != 2) {
    return fail("unexpected route count");
  }
  if (routes->routes[0].response_kind != NG_RESPONSE_RENDER ||
      strcmp(routes->routes[0].path, "/variables-server") != 0 ||
      strcmp(routes->routes[0].template_name, "variables") != 0 ||
      routes->routes[0].local_count != 4) {
    return fail("render route parse mismatch");
  }
  if (routes->routes[1].response_kind != NG_RESPONSE_JSON ||
      routes->routes[1].status_code != 201 ||
      routes->routes[1].local_count != 2) {
    return fail("json route parse mismatch");
  }

  if (ejs_parser_parse_text("variables", template_source, template_file) != 0) {
    return fail("ejs parser rejected valid template");
  }
  if (template_file->node_count < 8 ||
      template_file->nodes[1].kind != NG_EJS_NODE_EXPR ||
      template_file->nodes[3].kind != NG_EJS_NODE_IF_OPEN ||
      template_file->nodes[4].kind != NG_EJS_NODE_TEXT ||
      template_file->nodes[5].kind != NG_EJS_NODE_RAW_EXPR ||
      template_file->nodes[7].kind != NG_EJS_NODE_IF_CLOSE) {
    return fail("unexpected ejs node sequence");
  }

  ng_ejs_template_set_init(templates);
  templates->templates[0] = *template_file;
  templates->template_count = 1;
  if (server_codegen_emit(routes, templates, 2, codegen) != 0) {
    return fail("server codegen failed");
  }

  if (strstr(codegen->support_source, "angular_render_variables_template") == NULL ||
      strstr(codegen->route_source, "angular_backend_route_0") == NULL ||
      strstr(codegen->route_source, "json_object_add_boolean(root, \"ok\", true);") == NULL ||
      strstr(codegen->route_init, "service->routes[2].path = \"/variables-server\";") == NULL) {
    return fail("generated backend code missing expected output");
  }

  free(routes);
  free(template_file);
  free(templates);
  free(codegen);
  return 0;
}
