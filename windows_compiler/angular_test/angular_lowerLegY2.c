#include "angular_lowerLegY2.h"
#include "helpers/include/math/number_utils.h"

const angular_lowerLegY2_header_t angular_lowerLegY2_header = {
  "AppComponent",
  "lowerLegY2",
  "field",
  "attribute-buffer",
  "char buffer[16]",
  "formatted attribute payload for SVG bindings",
  0
};

const char *angular_lowerLegY2_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "lowerLegY2", "274.0");
}
