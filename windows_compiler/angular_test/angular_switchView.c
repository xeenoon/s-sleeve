#include "angular_switchView.h"
#include "helpers/include/math/number_utils.h"

const angular_switchView_header_t angular_switchView_header = {
  "AppComponent",
  "switchView",
  "method",
  "generated-method",
  "void",
  "generated from parsed component method",
  0
};

void angular_switchView_call(ng_runtime_t *runtime, double view) {
  (void)runtime;
  ng_runtime_set_double(runtime, "selectedView", view);
}
