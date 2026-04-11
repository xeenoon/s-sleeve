#include "angular_clamp.h"
#include "helpers/include/math/number_utils.h"

const angular_clamp_header_t angular_clamp_header = {
  "AppComponent",
  "clamp",
  "method",
  "inferred-method",
  "double",
  "generated from parsed Angular method body with inferred local types",
  0
};

double angular_clamp_call(ng_runtime_t *runtime, double value, double min, double max) {
  (void)runtime;
  return ng_math_min(max, ng_math_max(min, value));
}
