#include "angular_uncontrolledDescent.h"
#include "helpers/include/math/number_utils.h"

const angular_uncontrolledDescent_header_t angular_uncontrolledDescent_header = {
  "AppComponent",
  "uncontrolledDescent",
  "field",
  "runtime-slot",
  "dynamic",
  "generated field accessor derived from initializer",
  0
};

const char *angular_uncontrolledDescent_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "uncontrolledDescent", "");
}
