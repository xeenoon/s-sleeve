#include "angular_mapPercentToAngle.h"
#include "angular_clamp.h"
#include "helpers/include/math/number_utils.h"

const angular_mapPercentToAngle_header_t angular_mapPercentToAngle_header = {
  "AppComponent",
  "mapPercentToAngle",
  "method",
  "inferred-method",
  "double",
  "generated from parsed Angular method body with inferred local types",
  0
};

double angular_mapPercentToAngle_call(ng_runtime_t *runtime, double percentStraight) {
  (void)runtime;
  double normalized = angular_clamp_call(runtime, (percentStraight / 100), 0, 1);
  return (78 + ((0 - 78) * normalized));
}
