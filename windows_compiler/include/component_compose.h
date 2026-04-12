#ifndef COMPONENT_COMPOSE_H
#define COMPONENT_COMPOSE_H

#include <stddef.h>

#include "component_registry.h"

int component_compose_html(const component_registry_t *registry,
                           const component_unit_t *root,
                           char *buffer,
                           size_t buffer_size);
int component_compose_css(const component_registry_t *registry,
                          const component_unit_t *root,
                          char *buffer,
                          size_t buffer_size);

#endif
