#ifndef ANGULAR_NGONINIT_H
#define ANGULAR_NGONINIT_H

#include "angular_runtime.h"

#include "helpers/include/io/fetch_runtime.h"
#include "helpers/include/math/ng_math.h"
#include "helpers/include/math/number_utils.h"
#include "helpers/include/format/number_format.h"
#include "helpers/include/runtime/app_runtime.h"

#define ANGULAR_ngOnInit_COMPONENT "AppComponent"
#define ANGULAR_ngOnInit_KIND "method"
#define ANGULAR_ngOnInit_RUNTIME_CATEGORY "lifecycle-hook"
#define ANGULAR_ngOnInit_STORAGE_TYPE "ng_init_fn_t"
#define ANGULAR_ngOnInit_PROCESSING_NOTES "initializes timers and performs the first render-oriented state setup"
#define ANGULAR_ngOnInit_REQUIRES_EXTERNAL_FETCH 0

typedef struct {
  const char *component_name;
  const char *member_name;
  const char *member_kind;
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} angular_ngOnInit_header_t;

extern const angular_ngOnInit_header_t angular_ngOnInit_header;

void angular_ngOnInit_call(ng_runtime_t *runtime);

#endif
