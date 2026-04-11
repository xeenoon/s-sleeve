#include "data/json_utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "data/json.h"

char *ng_build_runtime_json(const ng_runtime_t *runtime, const char *const *keys, size_t key_count) {
  json_data *root;
  char *json_text;
  size_t index;

  if (runtime == NULL || keys == NULL) {
    return NULL;
  }

  root = init_json_object();

  for (index = 0; index < key_count; ++index) {
    const char *key = keys[index];
    const ng_runtime_slot_t *slot;
    size_t slot_index;

    slot = NULL;
    for (slot_index = 0; slot_index < runtime->slot_count; ++slot_index) {
      if (strcmp(runtime->slots[slot_index].name, key) == 0) {
        slot = &runtime->slots[slot_index];
        break;
      }
    }

    if (slot == NULL) {
      continue;
    }

    switch (slot->type) {
      case NG_RUNTIME_VALUE_INT:
        json_push(root, init_kvp(key, init_json_number(slot->data.int_value)));
        break;
      case NG_RUNTIME_VALUE_DOUBLE:
        json_push(root, init_kvp(key, init_json_number(slot->data.double_value)));
        break;
      case NG_RUNTIME_VALUE_BOOL:
        json_push(root, init_kvp(key, init_json_boolean(slot->data.bool_value)));
        break;
      case NG_RUNTIME_VALUE_STRING:
        json_push(root, init_kvp(key, init_json_string(slot->data.string_value)));
        break;
      case NG_RUNTIME_VALUE_EMPTY:
      default:
        break;
    }
  }

  json_text = json_tostring(root);
  json_free(root);
  return json_text;
}

int ng_extract_reading_json(const char *json_text, int *out_reading) {
  const char *reading_key;
  const char *cursor;
  char buffer[32];
  size_t index = 0;

  if (json_text == NULL || out_reading == NULL) {
    return 1;
  }

  reading_key = strstr(json_text, "\"reading\"");
  if (reading_key == NULL) {
    return 1;
  }

  cursor = strchr(reading_key, ':');
  if (cursor == NULL) {
    return 1;
  }
  cursor += 1;

  while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
    cursor += 1;
  }

  if (*cursor == '-') {
    buffer[index++] = *cursor;
    cursor += 1;
  }

  while (*cursor != '\0' && isdigit((unsigned char)*cursor) && index + 1 < sizeof(buffer)) {
    buffer[index++] = *cursor;
    cursor += 1;
  }
  buffer[index] = '\0';

  if (index == 0 || (index == 1 && buffer[0] == '-')) {
    return 1;
  }

  *out_reading = atoi(buffer);
  return 0;
}
