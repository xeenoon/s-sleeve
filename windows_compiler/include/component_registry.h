#ifndef COMPONENT_REGISTRY_H
#define COMPONENT_REGISTRY_H

#include <stddef.h>

#include "ast.h"
#include "file_io.h"

#define COMPONENT_REGISTRY_MAX_COMPONENTS 32

typedef struct {
  char id[128];
  char selector[128];
  char ng_path[260];
  char html_path[260];
  char css_path[260];
  int has_component_ast;
  int has_html;
  int has_css;
  ast_component_file_t component_ast;
  file_buffer_t html_buffer;
  file_buffer_t css_buffer;
} component_unit_t;

typedef struct {
  component_unit_t components[COMPONENT_REGISTRY_MAX_COMPONENTS];
  size_t component_count;
  size_t root_index;
} component_registry_t;

void component_registry_init(component_registry_t *registry);
void component_registry_free(component_registry_t *registry);
component_unit_t *component_registry_get_or_create(component_registry_t *registry, const char *path);
component_unit_t *component_registry_find_by_id(component_registry_t *registry, const char *id);
const component_unit_t *component_registry_find_by_selector(const component_registry_t *registry, const char *selector);
int component_registry_is_component_path(const char *path);
int component_registry_validate(const component_registry_t *registry);
const component_unit_t *component_registry_root(const component_registry_t *registry);

#endif
