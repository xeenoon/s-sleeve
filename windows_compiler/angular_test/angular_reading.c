#include "angular_reading.h"
#include "helpers/include/math/number_utils.h"

const angular_reading_header_t angular_reading_header = {
  "AppComponent",
  "reading",
  "field",
  "state-slot",
  "int",
  "raw potentiometer reading mirrored into ng_app_state_t.reading",
  0
};

int angular_reading_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_int(runtime, "reading", 0);
}
