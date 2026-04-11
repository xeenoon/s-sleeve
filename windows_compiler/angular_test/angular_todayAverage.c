#include "angular_todayAverage.h"
#include "helpers/include/math/number_utils.h"

const angular_todayAverage_header_t angular_todayAverage_header = {
  "AppComponent",
  "todayAverage",
  "field",
  "runtime-slot",
  "dynamic",
  "generated field accessor derived from initializer",
  0
};

const char *angular_todayAverage_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_string(runtime, "todayAverage", "");
}
