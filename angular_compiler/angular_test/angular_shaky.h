#ifndef ANGULAR_SHAKY_H
#define ANGULAR_SHAKY_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_shaky_COMPONENT "AppComponent"
#define ANGULAR_shaky_KIND "field"
#define ANGULAR_shaky_RUNTIME_CATEGORY "generic-field"
#define ANGULAR_shaky_STORAGE_TYPE "opaque generated field"
#define ANGULAR_shaky_PROCESSING_NOTES "generated fallback metadata for unsupported component fields"
#define ANGULAR_shaky_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_shaky_header_t;

extern const angular_shaky_header_t angular_shaky_header;

void angular_shaky_generated_stub(void);

#endif
