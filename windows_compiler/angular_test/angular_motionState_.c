#include "angular_motionState_.h"
#include "helpers/include/math/number_utils.h"

const angular_motionState__header_t angular_motionState__header = {
  "AppComponent",
  "motionState$",
  "field",
  "observable",
  "source-defined observable",
  "derived from observable initializer",
  0
};

const char *angular_motionState__get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "motionState$", "");
}
