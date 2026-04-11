#include "angular_stepCount.h"
#include "helpers/include/math/number_utils.h"

const angular_stepCount_header_t angular_stepCount_header = {
  "AppComponent",
  "stepCount",
  "field",
  "runtime-slot",
  "dynamic",
  "generated field accessor derived from initializer",
  0
};

const char *angular_stepCount_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "stepCount", "");
}
