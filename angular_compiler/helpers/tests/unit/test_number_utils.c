#include "math/number_utils.h"
#include "support/test.h"

void test_number_utils(ng_test_context_t *context) {
  NG_ASSERT_DOUBLE_CLOSE(context, 0.0, ng_clamp_double(-4.0, 0.0, 1.0), 0.0001);
  NG_ASSERT_DOUBLE_CLOSE(context, 1.0, ng_clamp_double(5.0, 0.0, 1.0), 0.0001);
  NG_ASSERT_DOUBLE_CLOSE(context, 0.25, ng_clamp_double(0.25, 0.0, 1.0), 0.0001);
  NG_ASSERT_INT_EQ(context, 73, ng_round_to_int(72.6));
  NG_ASSERT_INT_EQ(context, -2, ng_round_to_int(-1.6));
}
