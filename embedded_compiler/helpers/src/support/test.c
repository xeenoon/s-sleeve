#include "support/test.h"

#include <math.h>

void ng_assert_double_close(ng_test_context_t *context,
                            double expected,
                            double actual,
                            double tolerance,
                            const char *file,
                            int line) {
  context->assertions += 1;
  if (fabs(expected - actual) > tolerance) {
    fprintf(stderr,
            "ASSERT DOUBLE FAILED: expected=%f actual=%f tolerance=%f (%s:%d)\n",
            expected,
            actual,
            tolerance,
            file,
            line);
    context->failed = 1;
  }
}
