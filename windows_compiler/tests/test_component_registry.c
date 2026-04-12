#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "component_registry.h"
#include "parser.h"

static int fail(const char *message) {
  fprintf(stderr, "test_component_registry failed: %s\n", message);
  return 1;
}

int test_component_registry(void) {
  component_registry_t *registry;
  component_unit_t *app_unit;
  component_unit_t *live_unit;
  ast_file_t ast;
  const char *app_source =
      "@Component({ selector: 'app-root', templateUrl: './app.component.html', styleUrls: ['./app.component.css'] })\n"
      "export class AppComponent { selectedView = 'live'; }\n";
  const char *live_source =
      "@Component({ selector: 'live-dashboard', templateUrl: './live-dashboard.component.html', styleUrls: ['./live-dashboard.component.css'] })\n"
      "export class LiveDashboardComponent {}\n";

  registry = (component_registry_t *)malloc(sizeof(*registry));
  if (registry == NULL) {
    return fail("failed to allocate registry");
  }
  component_registry_init(registry);

  app_unit = component_registry_get_or_create(registry, "angular_app\\app.component.ng");
  live_unit = component_registry_get_or_create(registry, "angular_app\\components\\live-dashboard.component.ng");
  if (app_unit == NULL || live_unit == NULL) {
    component_registry_free(registry);
    free(registry);
    return fail("registry get_or_create returned null");
  }

  if (parser_parse_file("angular_app\\app.component.ng", app_source, strlen(app_source), &ast) != 0) {
    component_registry_free(registry);
    free(registry);
    return fail("failed to parse app component");
  }
  app_unit->component_ast = ast.data.component;
  app_unit->has_component_ast = 1;
  strcpy(app_unit->selector, ast.data.component.selector);

  if (parser_parse_file("angular_app\\components\\live-dashboard.component.ng", live_source, strlen(live_source), &ast) != 0) {
    component_registry_free(registry);
    free(registry);
    return fail("failed to parse live component");
  }
  live_unit->component_ast = ast.data.component;
  live_unit->has_component_ast = 1;
  strcpy(live_unit->selector, ast.data.component.selector);

  if (strcmp(app_unit->component_ast.selector, "app-root") != 0) {
    component_registry_free(registry);
    free(registry);
    return fail("app selector mismatch");
  }

  if (strcmp(live_unit->component_ast.selector, "live-dashboard") != 0) {
    component_registry_free(registry);
    free(registry);
    return fail("child selector mismatch");
  }

  component_registry_free(registry);
  free(registry);
  return 0;
}
