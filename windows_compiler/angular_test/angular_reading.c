#include "angular_reading.h"
#include "helpers/include/math/number_utils.h"

const angular_reading_header_t angular_reading_header = {
  "AppComponent",
  "reading",
  "field",
  "runtime-slot",
  "dynamic",
  "generated field accessor derived from initializer",
  0
};

const char *angular_reading_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "reading", "");
}
