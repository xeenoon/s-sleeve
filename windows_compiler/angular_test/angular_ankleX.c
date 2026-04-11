#include "angular_ankleX.h"
#include "helpers/include/math/number_utils.h"

const angular_ankleX_header_t angular_ankleX_header = {
  "AppComponent",
  "ankleX",
  "field",
  "attribute-buffer",
  "char buffer[16]",
  "formatted attribute payload for SVG bindings",
  0
};

const char *angular_ankleX_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "ankleX", "140.0");
}
