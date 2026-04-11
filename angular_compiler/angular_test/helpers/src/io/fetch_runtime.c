#include "io/fetch_runtime.h"

#include <ctype.h>
#include <stdlib.h>

int ng_parse_sensor_text(const char *text, int *out_value) {
  char *end;
  long parsed;

  if (text == NULL || out_value == NULL) {
    return 1;
  }

  while (*text != '\0' && isspace((unsigned char)*text)) {
    text += 1;
  }

  parsed = strtol(text, &end, 10);
  if (text == end) {
    return 1;
  }

  while (*end != '\0') {
    if (!isspace((unsigned char)*end)) {
      return 1;
    }
    end += 1;
  }

  *out_value = (int)parsed;
  return 0;
}

int ng_refresh_sensor_value(ng_fetch_text_fn fetch_fn,
                            void *context,
                            const char *path,
                            int *out_value) {
  char buffer[128];
  size_t length = 0;

  if (fetch_fn == NULL || out_value == NULL) {
    return 1;
  }

  if (fetch_fn(context, path, buffer, sizeof(buffer), &length) != 0) {
    return 1;
  }

  if (length >= sizeof(buffer)) {
    return 1;
  }

  buffer[length] = '\0';
  return ng_parse_sensor_text(buffer, out_value);
}
