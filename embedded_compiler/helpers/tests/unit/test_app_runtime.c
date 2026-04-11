#include <string.h>

#include "runtime/app_runtime.h"
#include "support/test.h"

void test_app_runtime(ng_test_context_t *context) {
  ng_runtime_t runtime;

  ng_runtime_init(&runtime);
  NG_ASSERT_INT_EQ(context, 0, ng_runtime_set_int(&runtime, "reading", 3500));
  NG_ASSERT_INT_EQ(context, 0, ng_runtime_set_double(&runtime, "angleDeg", 26.0));
  NG_ASSERT_INT_EQ(context, 0, ng_runtime_set_bool(&runtime, "hasSignal", 1));
  NG_ASSERT_INT_EQ(context, 0, ng_runtime_set_string(&runtime, "lowerLegX2", "183.8"));

  NG_ASSERT_INT_EQ(context, 3500, ng_runtime_get_int(&runtime, "reading", 0));
  NG_ASSERT_TRUE(context, ng_runtime_get_double(&runtime, "angleDeg", 0.0) > 25.9);
  NG_ASSERT_INT_EQ(context, 1, ng_runtime_get_bool(&runtime, "hasSignal", 0));
  NG_ASSERT_TRUE(context, strcmp(ng_runtime_get_string(&runtime, "lowerLegX2", ""), "183.8") == 0);
}
