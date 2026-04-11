#ifndef ANGULAR_LOWERLEGX2_H
#define ANGULAR_LOWERLEGX2_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_lowerLegX2_COMPONENT "AppComponent"
#define ANGULAR_lowerLegX2_KIND "field"
#define ANGULAR_lowerLegX2_RUNTIME_CATEGORY "attribute-buffer"
#define ANGULAR_lowerLegX2_STORAGE_TYPE "char buffer[16]"
#define ANGULAR_lowerLegX2_PROCESSING_NOTES "formatted attribute payload for SVG bindings"
#define ANGULAR_lowerLegX2_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_lowerLegX2_header_t;

extern const angular_lowerLegX2_header_t angular_lowerLegX2_header;

const char *angular_lowerLegX2_get(const ng_runtime_t *runtime);

#endif
