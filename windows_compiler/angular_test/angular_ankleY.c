#include "angular_ankleY.h"
#include "helpers/include/math/number_utils.h"

const angular_ankleY_header_t angular_ankleY_header = {
  "AppComponent",
  "ankleY",
  "field",
  "runtime-slot",
  "dynamic",
  "generated field accessor derived from initializer",
  0
};

const char *angular_ankleY_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "ankleY", "");
}
