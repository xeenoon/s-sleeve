#include "angular_liveState_.h"
#include "helpers/include/math/number_utils.h"

const angular_liveState__header_t angular_liveState__header = {
  "AppComponent",
  "liveState$",
  "field",
  "observable",
  "source-defined observable",
  "derived from observable initializer",
  0
};

const char *angular_liveState__get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "liveState$", "");
}
