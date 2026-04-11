#include "angular_historyGoal_.h"
#include "helpers/include/math/number_utils.h"

const angular_historyGoal__header_t angular_historyGoal__header = {
  "AppComponent",
  "historyGoal$",
  "field",
  "observable",
  "source-defined observable",
  "derived from observable initializer",
  0
};

const char *angular_historyGoal__get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "historyGoal$", "");
}
