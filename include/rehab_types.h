#ifndef REHAB_TYPES_H
#define REHAB_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STORAGE_MAGIC 0x53534C56UL
#define MAX_HISTORY 180
#define DAILY_GOAL_SCORE 85

#define ENABLE_SAMPLE_HISTORY 1

typedef struct {
  uint32_t id;
  uint64_t timestamp_ms;
  float score;
  float shakiness_penalty;
  float descent_penalty;
  float compensation_penalty;
  float duration_ms;
  float descent_ms;
  float ascent_ms;
  float range;
  float start_percent;
  float end_percent;
  float min_percent;
  float max_percent;
  float descent_avg_speed;
  float ascent_avg_speed;
  float descent_peak_speed;
  float ascent_peak_speed;
  uint16_t oscillation_count;
} StepRecord;

typedef struct {
  uint32_t magic;
  uint32_t count;
  uint32_t next_id;
  StepRecord records[MAX_HISTORY];
} PersistedHistory;

typedef struct {
  int raw_reading;
  float normalized;
  float knee_angle_deg;
  float percent_straight;
  float speed_percent_per_sec;
  float accel_percent_per_sec2;
  bool in_step;
  bool time_synced;
  uint32_t step_count;
  float last_score;
} LiveMetrics;

typedef struct {
  bool active;
  bool reached_depth;
  float start_norm;
  float min_norm;
  float max_norm;
  float last_norm;
  float final_norm;
  float path_length;
  float descent_peak_speed;
  float ascent_peak_speed;
  uint16_t oscillation_count;
  int last_motion_sign;
  unsigned long started_ms;
  unsigned long min_reached_ms;
  unsigned long last_reversal_ms;
} WorkingStep;

#endif
