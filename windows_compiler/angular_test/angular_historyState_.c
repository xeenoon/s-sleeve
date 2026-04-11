#include "angular_historyState_.h"
#include "helpers/include/math/number_utils.h"

const angular_historyState__header_t angular_historyState__header = {
  "AppComponent",
  "historyState$",
  "field",
  "observable",
  "source-defined observable",
  "derived from observable initializer",
  0
};

const char *angular_historyState__get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "historyState$", "");
}
