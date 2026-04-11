#ifndef NG_APP_RUNTIME_H
#define NG_APP_RUNTIME_H

#include <stddef.h>

#define NG_RUNTIME_MAX_SLOTS 64
#define NG_RUNTIME_MAX_NAME 64
#define NG_RUNTIME_MAX_STRING 128

typedef enum {
  NG_RUNTIME_VALUE_EMPTY = 0,
  NG_RUNTIME_VALUE_INT,
  NG_RUNTIME_VALUE_DOUBLE,
  NG_RUNTIME_VALUE_BOOL,
  NG_RUNTIME_VALUE_STRING
} ng_runtime_value_type_t;

typedef struct {
  char name[NG_RUNTIME_MAX_NAME];
  ng_runtime_value_type_t type;
  union {
    int int_value;
    double double_value;
    int bool_value;
    char string_value[NG_RUNTIME_MAX_STRING];
  } data;
} ng_runtime_slot_t;

typedef struct {
  ng_runtime_slot_t slots[NG_RUNTIME_MAX_SLOTS];
  size_t slot_count;
} ng_runtime_t;

void ng_runtime_init(ng_runtime_t *runtime);
int ng_runtime_set_int(ng_runtime_t *runtime, const char *name, int value);
int ng_runtime_set_double(ng_runtime_t *runtime, const char *name, double value);
int ng_runtime_set_bool(ng_runtime_t *runtime, const char *name, int value);
int ng_runtime_set_string(ng_runtime_t *runtime, const char *name, const char *value);
int ng_runtime_get_int(const ng_runtime_t *runtime, const char *name, int default_value);
double ng_runtime_get_double(const ng_runtime_t *runtime, const char *name, double default_value);
int ng_runtime_get_bool(const ng_runtime_t *runtime, const char *name, int default_value);
const char *ng_runtime_get_string(const ng_runtime_t *runtime, const char *name, const char *default_value);

#endif
