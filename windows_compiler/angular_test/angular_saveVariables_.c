#include "angular_saveVariables_.h"
#include "helpers/include/math/number_utils.h"

const angular_saveVariables__header_t angular_saveVariables__header = {
  "AppComponent",
  "saveVariables$",
  "field",
  "observable",
  "source-defined observable",
  "derived from observable initializer",
  0
};

const char *angular_saveVariables__get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "saveVariables$", "");
}
