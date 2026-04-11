#include "angular_dailyAverageRows_.h"
#include "helpers/include/math/number_utils.h"

const angular_dailyAverageRows__header_t angular_dailyAverageRows__header = {
  "AppComponent",
  "dailyAverageRows$",
  "field",
  "observable",
  "source-defined observable",
  "derived from observable initializer",
  0
};

const char *angular_dailyAverageRows__get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "dailyAverageRows$", "");
}
