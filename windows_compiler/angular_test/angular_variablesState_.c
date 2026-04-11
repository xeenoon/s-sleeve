#include "angular_variablesState_.h"
#include "helpers/include/math/number_utils.h"

const angular_variablesState__header_t angular_variablesState__header = {
  "AppComponent",
  "variablesState$",
  "field",
  "observable",
  "source-defined observable",
  "derived from observable initializer",
  0
};

const char *angular_variablesState__get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "variablesState$", "");
}
