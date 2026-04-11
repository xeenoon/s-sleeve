#ifndef ANGULAR_CLAMP_H
#define ANGULAR_CLAMP_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_clamp_COMPONENT "AppComponent"
#define ANGULAR_clamp_KIND "method"
#define ANGULAR_clamp_RUNTIME_CATEGORY "helper-function"
#define ANGULAR_clamp_STORAGE_TYPE "float helper"
#define ANGULAR_clamp_PROCESSING_NOTES "pure bounds clamp helper used by derived calculations"
#define ANGULAR_clamp_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_clamp_header_t;

extern const angular_clamp_header_t angular_clamp_header;

double angular_clamp_call(ng_runtime_t *runtime, double value, double min_value, double max_value);

#endif
