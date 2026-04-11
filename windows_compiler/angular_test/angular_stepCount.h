#ifndef ANGULAR_STEPCOUNT_H
#define ANGULAR_STEPCOUNT_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_stepCount_COMPONENT "AppComponent"
#define ANGULAR_stepCount_KIND "field"
#define ANGULAR_stepCount_RUNTIME_CATEGORY "runtime-slot"
#define ANGULAR_stepCount_STORAGE_TYPE "dynamic"
#define ANGULAR_stepCount_PROCESSING_NOTES "generated field accessor derived from initializer"
#define ANGULAR_stepCount_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_stepCount_header_t;

extern const angular_stepCount_header_t angular_stepCount_header;

const char *angular_stepCount_get(const ng_runtime_t *runtime);

#endif
