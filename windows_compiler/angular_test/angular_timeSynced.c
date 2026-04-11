#include "angular_timeSynced.h"
#include "helpers/include/math/number_utils.h"

const angular_timeSynced_header_t angular_timeSynced_header = {
  "AppComponent",
  "timeSynced",
  "field",
  "runtime-slot",
  "dynamic",
  "generated field accessor derived from initializer",
  0
};

const char *angular_timeSynced_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "timeSynced", "");
}
