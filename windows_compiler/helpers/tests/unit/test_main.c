#include "support/test.h"

void test_number_utils(ng_test_context_t *context);
void test_ng_math(ng_test_context_t *context);
void test_number_format(ng_test_context_t *context);
void test_fetch_runtime(ng_test_context_t *context);
void test_app_runtime(ng_test_context_t *context);
void test_json_utils(ng_test_context_t *context);
void test_http_service(ng_test_context_t *context);

static int run_test(const char *name, void (*fn)(ng_test_context_t *context), int *assertion_total) {
  ng_test_context_t context = {0, 0};
  fn(&context);
  *assertion_total += context.assertions;
  if (context.failed) {
    fprintf(stderr, "TEST FAILED: %s\n", name);
    return 1;
  }

  printf("TEST PASSED: %s (%d assertions)\n", name, context.assertions);
  return 0;
}

int main(void) {
  int assertion_total = 0;
  int failed = 0;

  failed |= run_test("number_utils", test_number_utils, &assertion_total);
  failed |= run_test("ng_math", test_ng_math, &assertion_total);
  failed |= run_test("number_format", test_number_format, &assertion_total);
  failed |= run_test("fetch_runtime", test_fetch_runtime, &assertion_total);
  failed |= run_test("app_runtime", test_app_runtime, &assertion_total);
  failed |= run_test("json_utils", test_json_utils, &assertion_total);
  failed |= run_test("http_service", test_http_service, &assertion_total);

  if (failed) {
    fprintf(stderr, "UNIT TESTS FAILED (%d assertions)\n", assertion_total);
    return 1;
  }

  printf("ALL UNIT TESTS PASSED (%d assertions)\n", assertion_total);
  return 0;
}
