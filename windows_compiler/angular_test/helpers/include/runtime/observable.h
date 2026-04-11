#ifndef NG_OBSERVABLE_H
#define NG_OBSERVABLE_H

#include <stddef.h>

#define NG_OBSERVABLE_MAX_TOPICS 16
#define NG_OBSERVABLE_MAX_NAME 64
#define NG_OBSERVABLE_MAX_PAYLOAD 1024

typedef struct {
  char name[NG_OBSERVABLE_MAX_NAME];
  char payload[NG_OBSERVABLE_MAX_PAYLOAD];
  unsigned long version;
  int subscriber_count;
  int initialized;
} ng_observable_topic_t;

typedef struct {
  ng_observable_topic_t topics[NG_OBSERVABLE_MAX_TOPICS];
  size_t topic_count;
} ng_observable_bus_t;

void ng_observable_bus_init(ng_observable_bus_t *bus);
int ng_observable_publish(ng_observable_bus_t *bus, const char *name, const char *payload);
const ng_observable_topic_t *ng_observable_get(const ng_observable_bus_t *bus, const char *name);
int ng_observable_subscribe(ng_observable_bus_t *bus, const char *name);

#endif
