#ifndef ANGULAR_SELECTEDVIEW__H
#define ANGULAR_SELECTEDVIEW__H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_selectedView__COMPONENT "AppComponent"
#define ANGULAR_selectedView$_KIND "field"
#define ANGULAR_selectedView__RUNTIME_CATEGORY "observable"
#define ANGULAR_selectedView$_STORAGE_TYPE "source-defined observable"
#define ANGULAR_selectedView$_PROCESSING_NOTES "derived from observable initializer"
#define ANGULAR_selectedView__REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_selectedView__header_t;

extern const angular_selectedView__header_t angular_selectedView__header;

const char *angular_selectedView__get(const ng_runtime_t *runtime);

#endif
