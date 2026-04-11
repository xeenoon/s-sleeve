#ifndef ANGULAR_UPDATEKNEE_H
#define ANGULAR_UPDATEKNEE_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_updateKnee_COMPONENT "AppComponent"
#define ANGULAR_updateKnee_KIND "method"
#define ANGULAR_updateKnee_RUNTIME_CATEGORY "recompute-hook"
#define ANGULAR_updateKnee_STORAGE_TYPE "ng_recompute_fn_t-compatible helper"
#define ANGULAR_updateKnee_PROCESSING_NOTES "recomputes leg geometry and derived display state"
#define ANGULAR_updateKnee_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_updateKnee_header_t;

extern const angular_updateKnee_header_t angular_updateKnee_header;

void angular_updateKnee_call(ng_runtime_t *runtime, double percentStraight);

#endif
