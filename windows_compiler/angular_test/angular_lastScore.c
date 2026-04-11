#include "angular_lastScore.h"
#include "helpers/include/math/number_utils.h"

const angular_lastScore_header_t angular_lastScore_header = {
  "AppComponent",
  "lastScore",
  "field",
  "runtime-slot",
  "dynamic",
  "generated field accessor derived from initializer",
  0
};

const char *angular_lastScore_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "lastScore", "");
}
