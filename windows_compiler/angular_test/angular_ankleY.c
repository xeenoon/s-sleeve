#include "angular_ankleY.h"
#include "helpers/include/math/number_utils.h"

const angular_ankleY_header_t angular_ankleY_header = {
  "AppComponent",
  "ankleY",
  "field",
  "attribute-buffer",
  "char buffer[16]",
  "formatted attribute payload for SVG bindings",
  0
};

const char *angular_ankleY_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "ankleY", "274.0");
}
