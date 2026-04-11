#ifndef REHAB_METRICS_H
#define REHAB_METRICS_H

#include "rehab_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int min_reading;
  int max_reading;
  float bent_angle;
  float straight_angle;
  unsigned long sample_interval_ms;
  float filter_alpha;
  float motion_threshold;
  float step_range_threshold;
  float start_ready_threshold;
  float return_margin;
  unsigned long max_step_duration_ms;
} RehabConfig;

typedef struct {
  float filtered_norm;
  float previous_norm;
  float previous_velocity;
  unsigned long last_sample_ms;
} RehabTrackerState;

typedef uint64_t (*rehab_time_fn_t)(void *context);

void rehab_tracker_init(RehabTrackerState *tracker, int initial_reading, const RehabConfig *config);
float rehab_reading_to_normalized(int reading, const RehabConfig *config);
float rehab_normalized_to_angle(float normalized, const RehabConfig *config);
float rehab_today_average_score(const PersistedHistory *history,
                                bool time_synced,
                                uint64_t current_epoch_ms,
                                int timezone_offset_minutes);
int64_t rehab_local_day_key(uint64_t timestamp_ms, int timezone_offset_minutes);
void rehab_push_step_record(PersistedHistory *history, const StepRecord *record);
bool rehab_update_sensor_processing(RehabTrackerState *tracker,
                                    WorkingStep *working,
                                    LiveMetrics *live,
                                    PersistedHistory *history,
                                    int raw_reading,
                                    unsigned long now_ms,
                                    bool time_synced,
                                    int timezone_offset_minutes,
                                    rehab_time_fn_t current_epoch_ms_fn,
                                    void *time_context,
                                    const RehabConfig *config);

#ifdef __cplusplus
}
#endif

#endif
