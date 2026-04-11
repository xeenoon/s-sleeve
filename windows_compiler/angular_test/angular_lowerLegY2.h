#ifndef ANGULAR_LOWERLEGY2_H
#define ANGULAR_LOWERLEGY2_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_lowerLegY2_COMPONENT "AppComponent"
#define ANGULAR_lowerLegY2_KIND "field"
#define ANGULAR_lowerLegY2_RUNTIME_CATEGORY "attribute-buffer"
#define ANGULAR_lowerLegY2_STORAGE_TYPE "char buffer[16]"
#define ANGULAR_lowerLegY2_PROCESSING_NOTES "formatted attribute payload for SVG bindings"
#define ANGULAR_lowerLegY2_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_lowerLegY2_header_t;

extern const angular_lowerLegY2_header_t angular_lowerLegY2_header;

const char *angular_lowerLegY2_get(const ng_runtime_t *runtime);

#endif
