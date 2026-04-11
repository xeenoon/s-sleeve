#include "angular_percentStraight.h"
#include "helpers/include/math/number_utils.h"

const angular_percentStraight_header_t angular_percentStraight_header = {
  "AppComponent",
  "percentStraight",
  "field",
  "derived-state",
  "int",
  "derived percentage from normalized knee angle",
  0
};

int angular_percentStraight_get(const ng_runtime_t *runtime) {
  return ng_runtime_get_int(runtime, "percentStraight", 0);
}
