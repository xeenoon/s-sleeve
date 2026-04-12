#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "component_compose.h"

static int fail(const char *message) {
  fprintf(stderr, "test_component_compose failed: %s\n", message);
  return 1;
}

static char *dup_text(const char *text) {
  size_t length = strlen(text) + 1;
  char *copy = (char *)malloc(length);
  if (copy != NULL) {
    memcpy(copy, text, length);
  }
  return copy;
}

int test_component_compose(void) {
  component_registry_t *registry;
  component_unit_t *app_unit;
  component_unit_t *live_unit;
  char html[4096];
  char css[4096];

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
    return fail("failed to create component units");
  }

  strcpy(app_unit->selector, "app-root");
  strcpy(live_unit->selector, "live-dashboard");
  app_unit->has_component_ast = 1;
  live_unit->has_component_ast = 1;
  app_unit->has_html = 1;
  app_unit->has_css = 1;
  live_unit->has_html = 1;
  live_unit->has_css = 1;
  app_unit->html_buffer.data = dup_text("<!DOCTYPE html><html><body><live-dashboard></live-dashboard></body></html>");
  app_unit->css_buffer.data = dup_text(".shell { color: red; }");
  live_unit->html_buffer.data = dup_text("<section id=\"live-view\"><h2>Live</h2></section>");
  live_unit->css_buffer.data = dup_text(".live { color: blue; }");

  if (component_compose_html(registry, app_unit, html, sizeof(html)) != 0) {
    component_registry_free(registry);
    free(registry);
    return fail("html composition failed");
  }

  if (strstr(html, "<section id=\"live-view\">") == NULL || strstr(html, "<live-dashboard>") != NULL) {
    component_registry_free(registry);
    free(registry);
    return fail("child selector was not expanded");
  }

  if (component_compose_css(registry, app_unit, css, sizeof(css)) != 0) {
    component_registry_free(registry);
    free(registry);
    return fail("css composition failed");
  }

  if (strstr(css, ".shell { color: red; }") == NULL || strstr(css, ".live { color: blue; }") == NULL) {
    component_registry_free(registry);
    free(registry);
    return fail("css composition missing child or root css");
  }

  component_registry_free(registry);
  free(registry);
  return 0;
}
