#ifndef ANGULAR_STEPHISTORY_H
#define ANGULAR_STEPHISTORY_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_stepHistory_COMPONENT "AppComponent"
#define ANGULAR_stepHistory_KIND "field"
#define ANGULAR_stepHistory_RUNTIME_CATEGORY "generic-field"
#define ANGULAR_stepHistory_STORAGE_TYPE "opaque generated field"
#define ANGULAR_stepHistory_PROCESSING_NOTES "generated fallback metadata for unsupported component fields"
#define ANGULAR_stepHistory_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_stepHistory_header_t;

extern const angular_stepHistory_header_t angular_stepHistory_header;

void angular_stepHistory_generated_stub(void);

#endif
