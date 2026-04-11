#ifndef ANGULAR_REFRESHLIVE_H
#define ANGULAR_REFRESHLIVE_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_refreshLive_COMPONENT "AppComponent"
#define ANGULAR_refreshLive_KIND "method"
#define ANGULAR_refreshLive_RUNTIME_CATEGORY "generic-method"
#define ANGULAR_refreshLive_STORAGE_TYPE "opaque generated method"
#define ANGULAR_refreshLive_PROCESSING_NOTES "generated fallback metadata for unsupported component methods"
#define ANGULAR_refreshLive_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_refreshLive_header_t;

extern const angular_refreshLive_header_t angular_refreshLive_header;

void angular_refreshLive_generated_stub(void);

#endif
