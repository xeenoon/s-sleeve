#include "angular_updateKnee.h"
#include "angular_mapPercentToAngle.h"
#include "helpers/include/math/number_utils.h"

const angular_updateKnee_header_t angular_updateKnee_header = {
  "AppComponent",
  "updateKnee",
  "method",
  "inferred-method",
  "void",
  "generated from parsed Angular method body with inferred local types",
  0
};

void angular_updateKnee_call(ng_runtime_t *runtime, double percentStraight) {
  (void)runtime;
  double angle = angular_mapPercentToAngle_call(runtime, percentStraight);
  double radians = ((angle * NG_MATH_PI) / 180);
  int kneeX = 140;
  int kneeY = 174;
  int shinLength = 100;
  double ankleX = (kneeX + (ng_math_sin(radians) * shinLength));
  double ankleY = (kneeY + (ng_math_cos(radians) * shinLength));
  {
    char _fmt7[32];
    ng_format_fixed_1(ankleX, _fmt7, sizeof(_fmt7));
    ng_runtime_set_string(runtime, "lowerLegX2", _fmt7);
  }
  {
    char _fmt8[32];
    ng_format_fixed_1(ankleY, _fmt8, sizeof(_fmt8));
    ng_runtime_set_string(runtime, "lowerLegY2", _fmt8);
  }
  {
    char _fmt9[32];
    ng_format_fixed_1(ankleX, _fmt9, sizeof(_fmt9));
    ng_runtime_set_string(runtime, "ankleX", _fmt9);
  }
  {
    char _fmt10[32];
    ng_format_fixed_1(ankleY, _fmt10, sizeof(_fmt10));
    ng_runtime_set_string(runtime, "ankleY", _fmt10);
  }
}
