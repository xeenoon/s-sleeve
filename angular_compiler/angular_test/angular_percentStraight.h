#ifndef ANGULAR_PERCENTSTRAIGHT_H
#define ANGULAR_PERCENTSTRAIGHT_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_percentStraight_COMPONENT "AppComponent"
#define ANGULAR_percentStraight_KIND "field"
#define ANGULAR_percentStraight_RUNTIME_CATEGORY "derived-state"
#define ANGULAR_percentStraight_STORAGE_TYPE "int"
#define ANGULAR_percentStraight_PROCESSING_NOTES "derived percentage from normalized knee angle"
#define ANGULAR_percentStraight_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_percentStraight_header_t;

extern const angular_percentStraight_header_t angular_percentStraight_header;

int angular_percentStraight_get(const ng_runtime_t *runtime);

#endif
