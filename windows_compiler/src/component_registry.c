#include "component_registry.h"

#include <string.h>

static int path_has_suffix(const char *path, const char *suffix) {
  size_t path_length = strlen(path);
  size_t suffix_length = strlen(suffix);
  if (path_length < suffix_length) {
    return 0;
  }
  return strcmp(path + path_length - suffix_length, suffix) == 0;
}

static const char *path_basename_ptr(const char *path) {
  const char *slash = strrchr(path, '\\');
  const char *forward = strrchr(path, '/');
  const char *base = slash;
  if (forward != NULL && (base == NULL || forward > base)) {
    base = forward;
  }
  return base != NULL ? base + 1 : path;
}

static void component_registry_extract_id(const char *path, char *buffer, size_t buffer_size) {
  const char *base = path_basename_ptr(path);
  const char *suffixes[] = {".component.ng", ".component.html", ".component.css"};
  size_t index;

  if (buffer_size == 0) {
    return;
  }
  buffer[0] = '\0';

  for (index = 0; index < sizeof(suffixes) / sizeof(suffixes[0]); ++index) {
    size_t suffix_length = strlen(suffixes[index]);
    size_t base_length = strlen(base);
    if (base_length > suffix_length && strcmp(base + base_length - suffix_length, suffixes[index]) == 0) {
      size_t id_length = base_length - suffix_length;
      if (id_length >= buffer_size) {
        id_length = buffer_size - 1;
      }
      memcpy(buffer, base, id_length);
      buffer[id_length] = '\0';
      return;
    }
  }
}

void component_registry_init(component_registry_t *registry) {
  memset(registry, 0, sizeof(*registry));
}

void component_registry_free(component_registry_t *registry) {
  size_t index;
  for (index = 0; index < registry->component_count; ++index) {
    file_buffer_free(&registry->components[index].html_buffer);
    file_buffer_free(&registry->components[index].css_buffer);
  }
  memset(registry, 0, sizeof(*registry));
}

component_unit_t *component_registry_find_by_id(component_registry_t *registry, const char *id) {
  size_t index;
  for (index = 0; index < registry->component_count; ++index) {
    if (strcmp(registry->components[index].id, id) == 0) {
      return &registry->components[index];
    }
  }
  return NULL;
}

const component_unit_t *component_registry_find_by_selector(const component_registry_t *registry, const char *selector) {
  size_t index;
  for (index = 0; index < registry->component_count; ++index) {
    if (strcmp(registry->components[index].selector, selector) == 0) {
      return &registry->components[index];
    }
  }
  return NULL;
}

component_unit_t *component_registry_get_or_create(component_registry_t *registry, const char *path) {
  char id[128];
  component_unit_t *unit;

  component_registry_extract_id(path, id, sizeof(id));
  if (id[0] == '\0') {
    return NULL;
  }

  unit = component_registry_find_by_id(registry, id);
  if (unit != NULL) {
    return unit;
  }

  if (registry->component_count >= COMPONENT_REGISTRY_MAX_COMPONENTS) {
    return NULL;
  }

  unit = &registry->components[registry->component_count++];
  memset(unit, 0, sizeof(*unit));
  strcpy(unit->id, id);
  if (strcmp(id, "app") == 0) {
    registry->root_index = registry->component_count - 1;
  }
  return unit;
}

int component_registry_is_component_path(const char *path) {
  return path_has_suffix(path, ".component.ng") ||
         path_has_suffix(path, ".component.html") ||
         path_has_suffix(path, ".component.css");
}

int component_registry_validate(const component_registry_t *registry) {
  size_t index;
  if (registry->component_count == 0) {
    return 1;
  }
  for (index = 0; index < registry->component_count; ++index) {
    const component_unit_t *unit = &registry->components[index];
    if (!unit->has_component_ast || !unit->has_html || !unit->has_css) {
      return 1;
    }
    if (unit->component_ast.selector[0] == '\0') {
      return 1;
    }
  }
  return 0;
}

const component_unit_t *component_registry_root(const component_registry_t *registry) {
  if (registry->component_count == 0 || registry->root_index >= registry->component_count) {
    return NULL;
  }
  return &registry->components[registry->root_index];
}
