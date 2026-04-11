#include "angular_compensation.h"
#include "helpers/include/math/number_utils.h"

const angular_compensation_header_t angular_compensation_header = {
  "AppComponent",
  "compensation",
  "field",
  "runtime-slot",
  "dynamic",
  "generated field accessor derived from initializer",
  0
};

const char *angular_compensation_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "compensation", "");
}
