#include "format/number_format.h"

#include <stdio.h>

int ng_format_fixed_1(double value, char *buffer, size_t buffer_size) {
  int written;

  if (buffer == NULL || buffer_size == 0) {
    return 1;
  }

  written = snprintf(buffer, buffer_size, "%.1f", value);
  if (written < 0 || (size_t)written >= buffer_size) {
    return 1;
  }

  return 0;
}
