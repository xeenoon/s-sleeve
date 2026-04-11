#include "angular_dailyDiagnostics_.h"
#include "helpers/include/math/number_utils.h"

const angular_dailyDiagnostics__header_t angular_dailyDiagnostics__header = {
  "AppComponent",
  "dailyDiagnostics$",
  "field",
  "observable",
  "source-defined observable",
  "derived from observable initializer",
  0
};

const char *angular_dailyDiagnostics__get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "dailyDiagnostics$", "");
}
