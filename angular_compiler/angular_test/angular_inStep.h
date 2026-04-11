#ifndef ANGULAR_INSTEP_H
#define ANGULAR_INSTEP_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_inStep_COMPONENT "AppComponent"
#define ANGULAR_inStep_KIND "field"
#define ANGULAR_inStep_RUNTIME_CATEGORY "generic-field"
#define ANGULAR_inStep_STORAGE_TYPE "opaque generated field"
#define ANGULAR_inStep_PROCESSING_NOTES "generated fallback metadata for unsupported component fields"
#define ANGULAR_inStep_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_inStep_header_t;

extern const angular_inStep_header_t angular_inStep_header;

void angular_inStep_generated_stub(void);

#endif
