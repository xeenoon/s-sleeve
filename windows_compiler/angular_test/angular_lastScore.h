#ifndef ANGULAR_LASTSCORE_H
#define ANGULAR_LASTSCORE_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_lastScore_COMPONENT "AppComponent"
#define ANGULAR_lastScore_KIND "field"
#define ANGULAR_lastScore_RUNTIME_CATEGORY "generic-field"
#define ANGULAR_lastScore_STORAGE_TYPE "opaque generated field"
#define ANGULAR_lastScore_PROCESSING_NOTES "generated fallback metadata for unsupported component fields"
#define ANGULAR_lastScore_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_lastScore_header_t;

extern const angular_lastScore_header_t angular_lastScore_header;

void angular_lastScore_generated_stub(void);

#endif
