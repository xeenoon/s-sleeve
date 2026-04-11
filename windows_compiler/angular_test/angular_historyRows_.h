#ifndef ANGULAR_HISTORYROWS__H
#define ANGULAR_HISTORYROWS__H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_historyRows__COMPONENT "AppComponent"
#define ANGULAR_historyRows$_KIND "field"
#define ANGULAR_historyRows__RUNTIME_CATEGORY "observable"
#define ANGULAR_historyRows$_STORAGE_TYPE "source-defined observable"
#define ANGULAR_historyRows$_PROCESSING_NOTES "derived from observable initializer"
#define ANGULAR_historyRows__REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_historyRows__header_t;

extern const angular_historyRows__header_t angular_historyRows__header;

const char *angular_historyRows__get(const ng_runtime_t *runtime);

#endif
