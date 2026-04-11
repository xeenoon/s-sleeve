#ifndef ANGULAR_SWITCHVIEW_H
#define ANGULAR_SWITCHVIEW_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_switchView_COMPONENT "AppComponent"
#define ANGULAR_switchView_KIND "method"
#define ANGULAR_switchView_RUNTIME_CATEGORY "generic-method"
#define ANGULAR_switchView_STORAGE_TYPE "opaque generated method"
#define ANGULAR_switchView_PROCESSING_NOTES "generated fallback metadata for unsupported component methods"
#define ANGULAR_switchView_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_switchView_header_t;

extern const angular_switchView_header_t angular_switchView_header;

void angular_switchView_call(ng_runtime_t *runtime, double view);

#endif
