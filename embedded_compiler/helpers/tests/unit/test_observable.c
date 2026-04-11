#include "runtime/observable.h"
#include "support/test.h"

void test_observable(ng_test_context_t *context) {
  ng_observable_bus_t bus;
  const ng_observable_topic_t *topic;

  ng_observable_bus_init(&bus);
  NG_ASSERT_INT_EQ(context, 0, (int)bus.topic_count);

  NG_ASSERT_INT_EQ(context, 0, ng_observable_subscribe(&bus, "liveState$"));
  NG_ASSERT_INT_EQ(context, 0, ng_observable_publish(&bus, "liveState$", "{\"reading\":3500}"));

  topic = ng_observable_get(&bus, "liveState$");
  NG_ASSERT_TRUE(context, topic != NULL);
  NG_ASSERT_INT_EQ(context, 1, topic->subscriber_count);
  NG_ASSERT_INT_EQ(context, 1, (int)topic->version);
  NG_ASSERT_TRUE(context, topic->initialized);
  NG_ASSERT_STR_EQ(context, "{\"reading\":3500}", topic->payload);

  NG_ASSERT_INT_EQ(context, 0, ng_observable_publish(&bus, "liveState$", "{\"reading\":3600}"));
  topic = ng_observable_get(&bus, "liveState$");
  NG_ASSERT_TRUE(context, topic != NULL);
  NG_ASSERT_INT_EQ(context, 2, (int)topic->version);
  NG_ASSERT_STR_EQ(context, "{\"reading\":3600}", topic->payload);
}
