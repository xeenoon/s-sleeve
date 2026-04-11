#include "angular_lowerLegX2.h"
#include "helpers/include/math/number_utils.h"

const angular_lowerLegX2_header_t angular_lowerLegX2_header = {
  "AppComponent",
  "lowerLegX2",
  "field",
  "attribute-buffer",
  "char buffer[16]",
  "formatted attribute payload for SVG bindings",
  0
};

const char *angular_lowerLegX2_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "lowerLegX2", "140.0");
}
