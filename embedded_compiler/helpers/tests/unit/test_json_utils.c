#include <stdlib.h>
#include <string.h>

#include "data/json_utils.h"
#include "runtime/app_runtime.h"
#include "support/test.h"

void test_json_utils(ng_test_context_t *context) {
  ng_runtime_t runtime;
  const char *keys[] = {"reading", "hasSignal", "lowerLegX2"};
  char *json_text;
  int reading;

  ng_runtime_init(&runtime);
  ng_runtime_set_int(&runtime, "reading", 3321);
  ng_runtime_set_bool(&runtime, "hasSignal", 1);
  ng_runtime_set_string(&runtime, "lowerLegX2", "183.8");
  json_text = ng_build_runtime_json(&runtime, keys, sizeof(keys) / sizeof(keys[0]));

  NG_ASSERT_TRUE(context, json_text != NULL);
  NG_ASSERT_TRUE(context, strstr(json_text, "\"reading\":3321") != NULL);
  NG_ASSERT_TRUE(context, strstr(json_text, "\"hasSignal\":true") != NULL);
  NG_ASSERT_TRUE(context, strstr(json_text, "\"lowerLegX2\":\"183.8\"") != NULL);
  NG_ASSERT_INT_EQ(context, 0, ng_extract_reading_json("{\"reading\":3650}", &reading));
  NG_ASSERT_INT_EQ(context, 3650, reading);
  NG_ASSERT_INT_EQ(context, 1, ng_extract_reading_json("{\"bad\":1}", &reading));

  free(json_text);
}
