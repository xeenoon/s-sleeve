#ifndef ANGULAR_HISTORYSTATE__H
#define ANGULAR_HISTORYSTATE__H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_historyState__COMPONENT "AppComponent"
#define ANGULAR_historyState$_KIND "field"
#define ANGULAR_historyState__RUNTIME_CATEGORY "observable"
#define ANGULAR_historyState$_STORAGE_TYPE "source-defined observable"
#define ANGULAR_historyState$_PROCESSING_NOTES "derived from observable initializer"
#define ANGULAR_historyState__REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_historyState__header_t;

extern const angular_historyState__header_t angular_historyState__header;

const char *angular_historyState__get(const ng_runtime_t *runtime);

#endif
