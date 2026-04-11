#include "angular_selectedView.h"
#include "helpers/include/math/number_utils.h"

const angular_selectedView_header_t angular_selectedView_header = {
  "AppComponent",
  "selectedView",
  "field",
  "runtime-slot",
  "dynamic",
  "generated field accessor derived from initializer",
  0
};

const char *angular_selectedView_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "selectedView", "");
}
