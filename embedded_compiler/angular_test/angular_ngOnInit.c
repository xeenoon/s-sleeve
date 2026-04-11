#include "angular_ngOnInit.h"
#include "helpers/include/math/number_utils.h"

const angular_ngOnInit_header_t angular_ngOnInit_header = {
  "AppComponent",
  "ngOnInit",
  "method",
  "lifecycle-hook",
  "ng_init_fn_t",
  "initializes timers and performs the first render-oriented state setup",
  0
};

void angular_ngOnInit_call(ng_runtime_t *runtime) {
  ng_runtime_init(runtime);
  ng_runtime_set_string(runtime, "selectedView", "live");
  ng_runtime_set_int(runtime, "goal", 85);
  ng_runtime_set_string(runtime, "lowerLegX2", "140.0");
  ng_runtime_set_string(runtime, "lowerLegY2", "274.0");
  ng_runtime_set_string(runtime, "ankleX", "140.0");
  ng_runtime_set_string(runtime, "ankleY", "274.0");
  ng_runtime_set_bool(runtime, "hasSignal", 0);
}
