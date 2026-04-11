#include "angular_goal.h"
#include "helpers/include/math/number_utils.h"

const angular_goal_header_t angular_goal_header = {
  "AppComponent",
  "goal",
  "field",
  "runtime-slot",
  "dynamic",
  "generated field accessor derived from initializer",
  0
};

const char *angular_goal_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "goal", "");
}
