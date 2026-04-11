#include "angular_selectedView_.h"
#include "helpers/include/math/number_utils.h"

const angular_selectedView__header_t angular_selectedView__header = {
  "AppComponent",
  "selectedView$",
  "field",
  "observable",
  "source-defined observable",
  "derived from observable initializer",
  0
};

const char *angular_selectedView__get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "selectedView$", "");
}
