#include "angular_shaky.h"
#include "helpers/include/math/number_utils.h"

const angular_shaky_header_t angular_shaky_header = {
  "AppComponent",
  "shaky",
  "field",
  "runtime-slot",
  "dynamic",
  "generated field accessor derived from initializer",
  0
};

const char *angular_shaky_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "shaky", "");
}
