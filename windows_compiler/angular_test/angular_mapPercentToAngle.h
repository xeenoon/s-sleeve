#ifndef ANGULAR_MAPPERCENTTOANGLE_H
#define ANGULAR_MAPPERCENTTOANGLE_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_mapPercentToAngle_COMPONENT "AppComponent"
#define ANGULAR_mapPercentToAngle_KIND "method"
#define ANGULAR_mapPercentToAngle_RUNTIME_CATEGORY "generated-method"
#define ANGULAR_mapPercentToAngle_STORAGE_TYPE "callable"
#define ANGULAR_mapPercentToAngle_PROCESSING_NOTES "generated from parsed component method"
#define ANGULAR_mapPercentToAngle_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_mapPercentToAngle_header_t;

extern const angular_mapPercentToAngle_header_t angular_mapPercentToAngle_header;

double angular_mapPercentToAngle_call(ng_runtime_t *runtime, double percentStraight);

#endif
