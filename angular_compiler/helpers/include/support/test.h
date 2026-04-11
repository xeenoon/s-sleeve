#ifndef NG_TEST_H
#define NG_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int failed;
  int assertions;
} ng_test_context_t;

#define NG_ASSERT_TRUE(ctx, expr)                                                      \
  do {                                                                                 \
    (ctx)->assertions += 1;                                                            \
    if (!(expr)) {                                                                     \
      fprintf(stderr, "ASSERT TRUE FAILED: %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
      (ctx)->failed = 1;                                                               \
      return;                                                                          \
    }                                                                                  \
  } while (0)

#define NG_ASSERT_INT_EQ(ctx, expected, actual)                                             \
  do {                                                                                      \
    int ng_expected_value = (expected);                                                     \
    int ng_actual_value = (actual);                                                         \
    (ctx)->assertions += 1;                                                                 \
    if (ng_expected_value != ng_actual_value) {                                             \
      fprintf(stderr,                                                                       \
              "ASSERT INT FAILED: expected=%d actual=%d (%s:%d)\n",                        \
              ng_expected_value,                                                            \
              ng_actual_value,                                                              \
              __FILE__,                                                                     \
              __LINE__);                                                                    \
      (ctx)->failed = 1;                                                                    \
      return;                                                                               \
    }                                                                                       \
  } while (0)

#define NG_ASSERT_STR_EQ(ctx, expected, actual)                                                \
  do {                                                                                         \
    const char *ng_expected_text = (expected);                                                 \
    const char *ng_actual_text = (actual);                                                     \
    (ctx)->assertions += 1;                                                                    \
    if (strcmp(ng_expected_text, ng_actual_text) != 0) {                                       \
      fprintf(stderr,                                                                          \
              "ASSERT STR FAILED: expected=%s actual=%s (%s:%d)\n",                           \
              ng_expected_text,                                                                \
              ng_actual_text,                                                                  \
              __FILE__,                                                                        \
              __LINE__);                                                                       \
      (ctx)->failed = 1;                                                                       \
      return;                                                                                  \
    }                                                                                          \
  } while (0)

void ng_assert_double_close(ng_test_context_t *context,
                            double expected,
                            double actual,
                            double tolerance,
                            const char *file,
                            int line);

#define NG_ASSERT_DOUBLE_CLOSE(ctx, expected, actual, tolerance) \
  ng_assert_double_close((ctx), (expected), (actual), (tolerance), __FILE__, __LINE__)

#endif
