#include "runtime/observable.h"

#include <stdio.h>
#include <string.h>

#ifndef NG_OBSERVABLE_TRACE_ENABLED
#define NG_OBSERVABLE_TRACE_ENABLED 1
#endif

#if NG_OBSERVABLE_TRACE_ENABLED
#define NG_OBSERVABLE_TRACE(...)   \
  do {                             \
    fprintf(stdout, __VA_ARGS__);  \
    fflush(stdout);                \
  } while (0)
#else
#define NG_OBSERVABLE_TRACE(...) \
  do {                           \
  } while (0)
#endif

static ng_observable_topic_t *ng_observable_find_mutable(ng_observable_bus_t *bus, const char *name) {
  size_t index;

  if (bus == NULL || name == NULL) {
    return NULL;
  }

  for (index = 0; index < bus->topic_count; ++index) {
    if (strcmp(bus->topics[index].name, name) == 0) {
      return &bus->topics[index];
    }
  }

  return NULL;
}

static ng_observable_topic_t *ng_observable_ensure_topic(ng_observable_bus_t *bus, const char *name) {
  ng_observable_topic_t *topic = ng_observable_find_mutable(bus, name);

  if (topic != NULL) {
    return topic;
  }

  if (bus == NULL || name == NULL || bus->topic_count >= NG_OBSERVABLE_MAX_TOPICS) {
    return NULL;
  }

  topic = &bus->topics[bus->topic_count];
  memset(topic, 0, sizeof(*topic));
  snprintf(topic->name, sizeof(topic->name), "%s", name);
  bus->topic_count += 1;
  NG_OBSERVABLE_TRACE("[observable] created topic=%s\n", topic->name);
  return topic;
}

void ng_observable_bus_init(ng_observable_bus_t *bus) {
  if (bus == NULL) {
    return;
  }

  memset(bus, 0, sizeof(*bus));
  NG_OBSERVABLE_TRACE("[observable] bus init\n");
}

int ng_observable_publish(ng_observable_bus_t *bus, const char *name, const char *payload) {
  ng_observable_topic_t *topic = ng_observable_ensure_topic(bus, name);

  if (topic == NULL) {
    return 1;
  }

  snprintf(topic->payload, sizeof(topic->payload), "%s", payload != NULL ? payload : "");
  topic->version += 1;
  topic->initialized = 1;
  NG_OBSERVABLE_TRACE("[observable] publish topic=%s version=%lu payload=%s\n",
                      topic->name,
                      topic->version,
                      topic->payload);
  return 0;
}

const ng_observable_topic_t *ng_observable_get(const ng_observable_bus_t *bus, const char *name) {
  size_t index;

  if (bus == NULL || name == NULL) {
    return NULL;
  }

  for (index = 0; index < bus->topic_count; ++index) {
    if (strcmp(bus->topics[index].name, name) == 0) {
      return &bus->topics[index];
    }
  }

  return NULL;
}

int ng_observable_subscribe(ng_observable_bus_t *bus, const char *name) {
  ng_observable_topic_t *topic = ng_observable_ensure_topic(bus, name);

  if (topic == NULL) {
    return 1;
  }

  topic->subscriber_count += 1;
  NG_OBSERVABLE_TRACE("[observable] subscribe topic=%s subscribers=%d\n",
                      topic->name,
                      topic->subscriber_count);
  return 0;
}
