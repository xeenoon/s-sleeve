#include "angular_historyRows_.h"
#include "helpers/include/math/number_utils.h"

const angular_historyRows__header_t angular_historyRows__header = {
  "AppComponent",
  "historyRows$",
  "field",
  "observable",
  "source-defined observable",
  "derived from observable initializer",
  0
};

const char *angular_historyRows__get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "historyRows$", "");
}
