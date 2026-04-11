#include "format/number_format.h"
#include "support/test.h"

void test_number_format(ng_test_context_t *context) {
  char buffer[16];

  NG_ASSERT_INT_EQ(context, 0, ng_format_fixed_1(274.04, buffer, sizeof(buffer)));
  NG_ASSERT_STR_EQ(context, "274.0", buffer);
  NG_ASSERT_INT_EQ(context, 0, ng_format_fixed_1(202.96, buffer, sizeof(buffer)));
  NG_ASSERT_STR_EQ(context, "203.0", buffer);
}
