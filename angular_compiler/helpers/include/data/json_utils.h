#ifndef NG_JSON_UTILS_H
#define NG_JSON_UTILS_H

#include <stddef.h>

#include "runtime/app_runtime.h"

char *ng_build_runtime_json(const ng_runtime_t *runtime, const char *const *keys, size_t key_count);
int ng_extract_reading_json(const char *json_text, int *out_reading);

#endif
