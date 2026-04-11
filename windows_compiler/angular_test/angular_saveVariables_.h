#ifndef ANGULAR_SAVEVARIABLES__H
#define ANGULAR_SAVEVARIABLES__H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_saveVariables__COMPONENT "AppComponent"
#define ANGULAR_saveVariables$_KIND "field"
#define ANGULAR_saveVariables__RUNTIME_CATEGORY "observable"
#define ANGULAR_saveVariables$_STORAGE_TYPE "source-defined observable"
#define ANGULAR_saveVariables$_PROCESSING_NOTES "derived from observable initializer"
#define ANGULAR_saveVariables__REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_saveVariables__header_t;

extern const angular_saveVariables__header_t angular_saveVariables__header;

const char *angular_saveVariables__get(const ng_runtime_t *runtime);

#endif
