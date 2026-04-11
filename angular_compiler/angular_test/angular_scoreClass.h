#ifndef ANGULAR_SCORECLASS_H
#define ANGULAR_SCORECLASS_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_scoreClass_COMPONENT "AppComponent"
#define ANGULAR_scoreClass_KIND "method"
#define ANGULAR_scoreClass_RUNTIME_CATEGORY "generic-method"
#define ANGULAR_scoreClass_STORAGE_TYPE "opaque generated method"
#define ANGULAR_scoreClass_PROCESSING_NOTES "generated fallback metadata for unsupported component methods"
#define ANGULAR_scoreClass_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_scoreClass_header_t;

extern const angular_scoreClass_header_t angular_scoreClass_header;

void angular_scoreClass_generated_stub(void);

#endif
