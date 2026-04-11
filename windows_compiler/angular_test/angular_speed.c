#include "angular_speed.h"
#include "helpers/include/math/number_utils.h"

const angular_speed_header_t angular_speed_header = {
  "AppComponent",
  "speed",
  "field",
  "runtime-slot",
  "dynamic",
  "generated field accessor derived from initializer",
  0
};

const char *angular_speed_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "speed", "");
}
