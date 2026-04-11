#include "math/ng_math.h"
#include "support/test.h"

void test_ng_math(ng_test_context_t *context) {
  NG_ASSERT_TRUE(context, ng_math_min(2.0, 5.0) == 2.0);
  NG_ASSERT_TRUE(context, ng_math_max(2.0, 5.0) == 5.0);
  NG_ASSERT_TRUE(context, ng_math_deg_to_rad(180.0) > 3.14 && ng_math_deg_to_rad(180.0) < 3.15);
  NG_ASSERT_TRUE(context, ng_math_sin(0.0) == 0.0);
  NG_ASSERT_TRUE(context, ng_math_cos(0.0) == 1.0);
}
