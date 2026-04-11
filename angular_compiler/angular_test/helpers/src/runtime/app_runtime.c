#include "runtime/app_runtime.h"

#include <stdio.h>
#include <string.h>

static ng_runtime_slot_t *ng_runtime_find_mutable(ng_runtime_t *runtime, const char *name) {
  size_t index;

  for (index = 0; index < runtime->slot_count; ++index) {
    if (strcmp(runtime->slots[index].name, name) == 0) {
      return &runtime->slots[index];
    }
  }

  if (runtime->slot_count >= NG_RUNTIME_MAX_SLOTS) {
    return NULL;
  }

  snprintf(runtime->slots[runtime->slot_count].name,
           sizeof(runtime->slots[runtime->slot_count].name),
           "%s",
           name);
  runtime->slots[runtime->slot_count].type = NG_RUNTIME_VALUE_EMPTY;
  runtime->slots[runtime->slot_count].data.string_value[0] = '\0';
  runtime->slot_count += 1;
  return &runtime->slots[runtime->slot_count - 1];
}

static const ng_runtime_slot_t *ng_runtime_find(const ng_runtime_t *runtime, const char *name) {
  size_t index;

  for (index = 0; index < runtime->slot_count; ++index) {
    if (strcmp(runtime->slots[index].name, name) == 0) {
      return &runtime->slots[index];
    }
  }

  return NULL;
}

void ng_runtime_init(ng_runtime_t *runtime) {
  if (runtime == NULL) {
    return;
  }

  memset(runtime, 0, sizeof(*runtime));
}

int ng_runtime_set_int(ng_runtime_t *runtime, const char *name, int value) {
  ng_runtime_slot_t *slot = ng_runtime_find_mutable(runtime, name);
  if (slot == NULL) {
    return 1;
  }
  slot->type = NG_RUNTIME_VALUE_INT;
  slot->data.int_value = value;
  return 0;
}

int ng_runtime_set_double(ng_runtime_t *runtime, const char *name, double value) {
  ng_runtime_slot_t *slot = ng_runtime_find_mutable(runtime, name);
  if (slot == NULL) {
    return 1;
  }
  slot->type = NG_RUNTIME_VALUE_DOUBLE;
  slot->data.double_value = value;
  return 0;
}

int ng_runtime_set_bool(ng_runtime_t *runtime, const char *name, int value) {
  ng_runtime_slot_t *slot = ng_runtime_find_mutable(runtime, name);
  if (slot == NULL) {
    return 1;
  }
  slot->type = NG_RUNTIME_VALUE_BOOL;
  slot->data.bool_value = value ? 1 : 0;
  return 0;
}

int ng_runtime_set_string(ng_runtime_t *runtime, const char *name, const char *value) {
  ng_runtime_slot_t *slot = ng_runtime_find_mutable(runtime, name);
  if (slot == NULL || value == NULL) {
    return 1;
  }
  slot->type = NG_RUNTIME_VALUE_STRING;
  snprintf(slot->data.string_value, sizeof(slot->data.string_value), "%s", value);
  return 0;
}

int ng_runtime_get_int(const ng_runtime_t *runtime, const char *name, int default_value) {
  const ng_runtime_slot_t *slot = ng_runtime_find(runtime, name);
  if (slot == NULL) {
    return default_value;
  }
  if (slot->type == NG_RUNTIME_VALUE_INT) {
    return slot->data.int_value;
  }
  if (slot->type == NG_RUNTIME_VALUE_DOUBLE) {
    return (int)slot->data.double_value;
  }
  if (slot->type == NG_RUNTIME_VALUE_BOOL) {
    return slot->data.bool_value;
  }
  return default_value;
}

double ng_runtime_get_double(const ng_runtime_t *runtime, const char *name, double default_value) {
  const ng_runtime_slot_t *slot = ng_runtime_find(runtime, name);
  if (slot == NULL) {
    return default_value;
  }
  if (slot->type == NG_RUNTIME_VALUE_DOUBLE) {
    return slot->data.double_value;
  }
  if (slot->type == NG_RUNTIME_VALUE_INT) {
    return (double)slot->data.int_value;
  }
  if (slot->type == NG_RUNTIME_VALUE_BOOL) {
    return (double)slot->data.bool_value;
  }
  return default_value;
}

int ng_runtime_get_bool(const ng_runtime_t *runtime, const char *name, int default_value) {
  const ng_runtime_slot_t *slot = ng_runtime_find(runtime, name);
  if (slot == NULL) {
    return default_value;
  }
  if (slot->type == NG_RUNTIME_VALUE_BOOL) {
    return slot->data.bool_value;
  }
  if (slot->type == NG_RUNTIME_VALUE_INT) {
    return slot->data.int_value != 0;
  }
  if (slot->type == NG_RUNTIME_VALUE_DOUBLE) {
    return slot->data.double_value != 0.0;
  }
  return default_value;
}

const char *ng_runtime_get_string(const ng_runtime_t *runtime, const char *name, const char *default_value) {
  const ng_runtime_slot_t *slot = ng_runtime_find(runtime, name);
  if (slot == NULL || slot->type != NG_RUNTIME_VALUE_STRING) {
    return default_value;
  }
  return slot->data.string_value;
}
