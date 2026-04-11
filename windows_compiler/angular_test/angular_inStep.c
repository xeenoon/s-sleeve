#include "angular_inStep.h"
#include "helpers/include/math/number_utils.h"

const angular_inStep_header_t angular_inStep_header = {
  "AppComponent",
  "inStep",
  "field",
  "runtime-slot",
  "dynamic",
  "generated field accessor derived from initializer",
  0
};

const char *angular_inStep_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "inStep", "");
}
