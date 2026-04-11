#include <string.h>

#include "io/fetch_runtime.h"
#include "support/test.h"

static int fake_fetch_success(void *context,
                              const char *path,
                              char *buffer,
                              size_t buffer_size,
                              size_t *out_length) {
  const char *text = (const char *)context;
  size_t length = strlen(text);
  (void)path;

  if (length >= buffer_size) {
    return 1;
  }

  memcpy(buffer, text, length);
  *out_length = length;
  return 0;
}

void test_fetch_runtime(ng_test_context_t *context) {
  int value = 0;

  NG_ASSERT_INT_EQ(context, 0, ng_parse_sensor_text("  3522\n", &value));
  NG_ASSERT_INT_EQ(context, 3522, value);
  NG_ASSERT_INT_EQ(context, 1, ng_parse_sensor_text("wat", &value));
  NG_ASSERT_INT_EQ(context,
                   0,
                   ng_refresh_sensor_value(fake_fetch_success, "3675", "/value", &value));
  NG_ASSERT_INT_EQ(context, 3675, value);
}
