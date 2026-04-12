#include "component_compose.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int append_text(char *buffer, size_t buffer_size, size_t *cursor, const char *text) {
  int written;
  if (*cursor >= buffer_size) {
    return 1;
  }
  written = snprintf(buffer + *cursor, buffer_size - *cursor, "%s", text != NULL ? text : "");
  if (written < 0 || (size_t)written >= buffer_size - *cursor) {
    return 1;
  }
  *cursor += (size_t)written;
  return 0;
}

static int append_format(char *buffer, size_t buffer_size, size_t *cursor, const char *format, ...) {
  va_list args;
  int written;
  if (*cursor >= buffer_size) {
    return 1;
  }
  va_start(args, format);
  written = vsnprintf(buffer + *cursor, buffer_size - *cursor, format, args);
  va_end(args);
  if (written < 0 || (size_t)written >= buffer_size - *cursor) {
    return 1;
  }
  *cursor += (size_t)written;
  return 0;
}

static const component_unit_t *find_component_by_tag(const component_registry_t *registry, const char *tag_name) {
  return component_registry_find_by_selector(registry, tag_name);
}

static int compose_component_html_recursive(const component_registry_t *registry,
                                            const component_unit_t *component,
                                            char *buffer,
                                            size_t buffer_size,
                                            size_t *cursor) {
  const char *html = component->html_buffer.data;
  size_t index = 0;

  while (html != NULL && html[index] != '\0') {
    if (html[index] == '<' && html[index + 1] != '/' && html[index + 1] != '!' && html[index + 1] != '?') {
      char tag_name[128];
      size_t tag_cursor = 0;
      size_t tag_end = index + 1;
      const component_unit_t *child;
      const char *close_tag;

      while (html[tag_end] != '\0' &&
             html[tag_end] != '>' &&
             html[tag_end] != ' ' &&
             html[tag_end] != '\t' &&
             html[tag_end] != '\r' &&
             html[tag_end] != '\n' &&
             html[tag_end] != '/') {
        if (tag_cursor + 1 < sizeof(tag_name)) {
          tag_name[tag_cursor++] = html[tag_end];
        }
        tag_end += 1;
      }
      tag_name[tag_cursor] = '\0';
      child = find_component_by_tag(registry, tag_name);
      if (child != NULL && child != component) {
        char close_pattern[160];
        const char *open_end = strchr(html + index, '>');
        if (open_end == NULL) {
          return 1;
        }
        snprintf(close_pattern, sizeof(close_pattern), "</%s>", tag_name);
        close_tag = strstr(open_end + 1, close_pattern);
        if (close_tag == NULL) {
          return 1;
        }
        if (compose_component_html_recursive(registry, child, buffer, buffer_size, cursor) != 0) {
          return 1;
        }
        index = (size_t)((close_tag - html) + strlen(close_pattern));
        continue;
      }
    }

    if (*cursor + 1 >= buffer_size) {
      return 1;
    }
    buffer[(*cursor)++] = html[index++];
  }

  buffer[*cursor] = '\0';
  return 0;
}

int component_compose_html(const component_registry_t *registry,
                           const component_unit_t *root,
                           char *buffer,
                           size_t buffer_size) {
  size_t cursor = 0;
  if (buffer_size == 0 || root == NULL) {
    return 1;
  }
  buffer[0] = '\0';
  return compose_component_html_recursive(registry, root, buffer, buffer_size, &cursor);
}

static int compose_component_css_recursive(const component_registry_t *registry,
                                           const component_unit_t *component,
                                           int *visited,
                                           char *buffer,
                                           size_t buffer_size,
                                           size_t *cursor) {
  const char *html = component->html_buffer.data;
  size_t index = 0;
  size_t component_index;

  for (component_index = 0; component_index < registry->component_count; ++component_index) {
    if (&registry->components[component_index] == component) {
      break;
    }
  }

  if (component_index >= registry->component_count) {
    return 1;
  }
  if (visited[component_index]) {
    return 0;
  }
  visited[component_index] = 1;

  if (append_format(buffer, buffer_size, cursor, "\n/* component:%s */\n", component->id) != 0 ||
      append_text(buffer, buffer_size, cursor, component->css_buffer.data) != 0 ||
      append_text(buffer, buffer_size, cursor, "\n") != 0) {
    return 1;
  }

  while (html != NULL && html[index] != '\0') {
    if (html[index] == '<' && html[index + 1] != '/' && html[index + 1] != '!' && html[index + 1] != '?') {
      char tag_name[128];
      size_t tag_cursor = 0;
      size_t tag_end = index + 1;
      const component_unit_t *child;

      while (html[tag_end] != '\0' &&
             html[tag_end] != '>' &&
             html[tag_end] != ' ' &&
             html[tag_end] != '\t' &&
             html[tag_end] != '\r' &&
             html[tag_end] != '\n' &&
             html[tag_end] != '/') {
        if (tag_cursor + 1 < sizeof(tag_name)) {
          tag_name[tag_cursor++] = html[tag_end];
        }
        tag_end += 1;
      }
      tag_name[tag_cursor] = '\0';
      child = find_component_by_tag(registry, tag_name);
      if (child != NULL && child != component) {
        if (compose_component_css_recursive(registry, child, visited, buffer, buffer_size, cursor) != 0) {
          return 1;
        }
      }
    }
    index += 1;
  }

  return 0;
}

int component_compose_css(const component_registry_t *registry,
                          const component_unit_t *root,
                          char *buffer,
                          size_t buffer_size) {
  int visited[COMPONENT_REGISTRY_MAX_COMPONENTS];
  size_t cursor = 0;
  memset(visited, 0, sizeof(visited));
  if (buffer_size == 0 || root == NULL) {
    return 1;
  }
  buffer[0] = '\0';
  return compose_component_css_recursive(registry, root, visited, buffer, buffer_size, &cursor);
}
