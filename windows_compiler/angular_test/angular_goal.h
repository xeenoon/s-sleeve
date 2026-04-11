#ifndef ANGULAR_GOAL_H
#define ANGULAR_GOAL_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_goal_COMPONENT "AppComponent"
#define ANGULAR_goal_KIND "field"
#define ANGULAR_goal_RUNTIME_CATEGORY "runtime-slot"
#define ANGULAR_goal_STORAGE_TYPE "dynamic"
#define ANGULAR_goal_PROCESSING_NOTES "generated field accessor derived from initializer"
#define ANGULAR_goal_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_goal_header_t;

extern const angular_goal_header_t angular_goal_header;

const char *angular_goal_get(const ng_runtime_t *runtime);

#endif
