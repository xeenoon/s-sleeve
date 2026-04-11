#include "angular_historyDiagnostics_.h"
#include "helpers/include/math/number_utils.h"

const angular_historyDiagnostics__header_t angular_historyDiagnostics__header = {
  "AppComponent",
  "historyDiagnostics$",
  "field",
  "observable",
  "source-defined observable",
  "derived from observable initializer",
  0
};

const char *angular_historyDiagnostics__get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "historyDiagnostics$", "");
}
