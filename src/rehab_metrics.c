#include "rehab_metrics.h"

#include <math.h>
#include <string.h>

static float clampf_local(float value, float minimum, float maximum) {
  if (value < minimum) {
    return minimum;
  }
  if (value > maximum) {
    return maximum;
  }
  return value;
}

static float maxf_local(float left, float right) {
  return left > right ? left : right;
}

static float minf_local(float left, float right) {
  return left < right ? left : right;
}

void rehab_tracker_init(RehabTrackerState *tracker, int initial_reading, const RehabConfig *config) {
  tracker->filtered_norm = rehab_reading_to_normalized(initial_reading, config);
  tracker->previous_norm = tracker->filtered_norm;
  tracker->previous_velocity = 0.0f;
  tracker->last_sample_ms = 0;
}

float rehab_reading_to_normalized(int reading, const RehabConfig *config) {
  const float scaled =
      (float)(reading - config->min_reading) / (float)(config->max_reading - config->min_reading);
  return clampf_local(scaled, 0.0f, 1.0f);
}

float rehab_normalized_to_angle(float normalized, const RehabConfig *config) {
  return config->bent_angle + (config->straight_angle - config->bent_angle) * normalized;
}

int64_t rehab_local_day_key(uint64_t timestamp_ms, int timezone_offset_minutes) {
  int64_t local_ms;

  if (timestamp_ms == 0) {
    return -1;
  }

  local_ms = (int64_t)timestamp_ms - (int64_t)timezone_offset_minutes * 60000LL;
  return local_ms / 86400000LL;
}

float rehab_today_average_score(const PersistedHistory *history,
                                bool time_synced,
                                uint64_t current_epoch_ms,
                                int timezone_offset_minutes) {
  float sum = 0.0f;
  int count = 0;
  size_t index;

  if (history->count == 0) {
    return 0.0f;
  }

  if (!time_synced) {
    return history->records[history->count - 1].score;
  }

  const int64_t today_key = rehab_local_day_key(current_epoch_ms, timezone_offset_minutes);
  for (index = 0; index < history->count; ++index) {
    if (rehab_local_day_key(history->records[index].timestamp_ms, timezone_offset_minutes) == today_key) {
      sum += history->records[index].score;
      ++count;
    }
  }

  return count > 0 ? sum / (float)count : 0.0f;
}

void rehab_push_step_record(PersistedHistory *history, const StepRecord *record) {
  if (history->count < MAX_HISTORY) {
    history->records[history->count++] = *record;
    return;
  }

  memmove(history->records, history->records + 1, sizeof(StepRecord) * (MAX_HISTORY - 1));
  history->records[MAX_HISTORY - 1] = *record;
}

static void begin_step(WorkingStep *working, LiveMetrics *live, float normalized, unsigned long now_ms, int motion_sign) {
  memset(working, 0, sizeof(*working));
  working->active = true;
  working->start_norm = normalized;
  working->min_norm = normalized;
  working->max_norm = normalized;
  working->last_norm = normalized;
  working->final_norm = normalized;
  working->started_ms = now_ms;
  working->last_reversal_ms = now_ms;
  working->last_motion_sign = motion_sign;
  live->in_step = true;
}

static bool finish_step(WorkingStep *working,
                        LiveMetrics *live,
                        PersistedHistory *history,
                        unsigned long now_ms,
                        int timezone_offset_minutes,
                        rehab_time_fn_t current_epoch_ms_fn,
                        void *time_context,
                        const RehabConfig *config) {
  StepRecord record;
  float amplitude = working->start_norm - working->min_norm;
  float total_ms;
  float descent_ms;
  float ascent_ms;
  float end_norm;
  float descent_distance;
  float ascent_distance;
  float ideal_path;
  float extra_path;
  float phase_imbalance;
  float speed_descent;
  float speed_ascent;
  float peak_descent;
  float peak_ascent;
  float speed_imbalance;
  float return_error;
  float shakiness_penalty;
  float descent_penalty;
  float compensation_penalty;
  float score;

  (void)timezone_offset_minutes;
  (void)config;

  if (amplitude < config->step_range_threshold) {
    memset(working, 0, sizeof(*working));
    live->in_step = false;
    return false;
  }

  total_ms = (float)(now_ms - working->started_ms);
  descent_ms = (float)(working->min_reached_ms - working->started_ms);
  ascent_ms = maxf_local(1.0f, total_ms - descent_ms);
  end_norm = working->final_norm;
  descent_distance = maxf_local(0.01f, working->start_norm - working->min_norm);
  ascent_distance = maxf_local(0.01f, end_norm - working->min_norm);
  ideal_path = descent_distance + ascent_distance;
  extra_path = maxf_local(0.0f, working->path_length - ideal_path);
  phase_imbalance = fabsf(descent_ms - ascent_ms) / maxf_local(1.0f, total_ms);
  speed_descent = descent_distance / maxf_local(0.08f, descent_ms / 1000.0f) * 100.0f;
  speed_ascent = ascent_distance / maxf_local(0.08f, ascent_ms / 1000.0f) * 100.0f;
  peak_descent = working->descent_peak_speed * 100.0f;
  peak_ascent = working->ascent_peak_speed * 100.0f;
  speed_imbalance =
      fabsf(speed_descent - speed_ascent) / maxf_local(maxf_local(speed_descent, speed_ascent), 1.0f);
  return_error = fabsf(end_norm - working->start_norm);

  shakiness_penalty =
      clampf_local(extra_path * 180.0f + (float)working->oscillation_count * 11.0f, 0.0f, 100.0f);
  descent_penalty = clampf_local(
      maxf_local(0.0f, speed_descent - 70.0f) * 0.55f +
          maxf_local(0.0f, peak_descent - speed_descent * 1.7f) * 0.35f,
      0.0f,
      100.0f);
  compensation_penalty = clampf_local(
      phase_imbalance * 70.0f + speed_imbalance * 45.0f + return_error * 140.0f,
      0.0f,
      100.0f);
  score = clampf_local(
      100.0f - (shakiness_penalty * 0.35f + descent_penalty * 0.35f + compensation_penalty * 0.30f),
      0.0f,
      100.0f);

  memset(&record, 0, sizeof(record));
  record.id = history->next_id++;
  record.timestamp_ms = current_epoch_ms_fn ? current_epoch_ms_fn(time_context) : 0;
  record.score = score;
  record.shakiness_penalty = shakiness_penalty;
  record.descent_penalty = descent_penalty;
  record.compensation_penalty = compensation_penalty;
  record.duration_ms = total_ms;
  record.descent_ms = descent_ms;
  record.ascent_ms = ascent_ms;
  record.range = amplitude * 100.0f;
  record.start_percent = working->start_norm * 100.0f;
  record.end_percent = end_norm * 100.0f;
  record.min_percent = working->min_norm * 100.0f;
  record.max_percent = working->max_norm * 100.0f;
  record.descent_avg_speed = speed_descent;
  record.ascent_avg_speed = speed_ascent;
  record.descent_peak_speed = peak_descent;
  record.ascent_peak_speed = peak_ascent;
  record.oscillation_count = working->oscillation_count;

  rehab_push_step_record(history, &record);
  live->step_count = history->count;
  live->last_score = score;
  memset(working, 0, sizeof(*working));
  live->in_step = false;
  return true;
}

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
                                    const RehabConfig *config) {
  float dt_seconds;
  float normalized;
  float velocity;
  float accel;
  int motion_sign = 0;

  if (tracker->last_sample_ms != 0 &&
      now_ms - tracker->last_sample_ms < config->sample_interval_ms) {
    return false;
  }

  dt_seconds = tracker->last_sample_ms == 0
                   ? ((float)config->sample_interval_ms / 1000.0f)
                   : maxf_local(0.001f, (float)(now_ms - tracker->last_sample_ms) / 1000.0f);
  tracker->last_sample_ms = now_ms;

  normalized = rehab_reading_to_normalized(raw_reading, config);
  tracker->filtered_norm =
      tracker->filtered_norm + (normalized - tracker->filtered_norm) * config->filter_alpha;
  velocity = (tracker->filtered_norm - tracker->previous_norm) / dt_seconds;
  accel = (velocity - tracker->previous_velocity) / dt_seconds;

  live->raw_reading = raw_reading;
  live->normalized = tracker->filtered_norm;
  live->knee_angle_deg = rehab_normalized_to_angle(tracker->filtered_norm, config);
  live->percent_straight = tracker->filtered_norm * 100.0f;
  live->speed_percent_per_sec = fabsf(velocity) * 100.0f;
  live->accel_percent_per_sec2 = fabsf(accel) * 100.0f;
  live->time_synced = time_synced;

  if (velocity <= -config->motion_threshold) {
    motion_sign = -1;
  } else if (velocity >= config->motion_threshold) {
    motion_sign = 1;
  }

  if (!working->active) {
    if (tracker->filtered_norm >= config->start_ready_threshold && motion_sign == -1) {
      begin_step(working, live, tracker->filtered_norm, now_ms, motion_sign);
    }
  } else {
    working->path_length += fabsf(tracker->filtered_norm - working->last_norm);
    working->last_norm = tracker->filtered_norm;
    working->final_norm = tracker->filtered_norm;
    working->min_norm = minf_local(working->min_norm, tracker->filtered_norm);
    working->max_norm = maxf_local(working->max_norm, tracker->filtered_norm);

    if (motion_sign != 0 && working->last_motion_sign != 0 &&
        motion_sign != working->last_motion_sign &&
        now_ms - working->last_reversal_ms > 120) {
      bool is_main_turn =
          !working->reached_depth && motion_sign == 1 &&
          (working->start_norm - working->min_norm) >= config->step_range_threshold;

      if (!is_main_turn) {
        ++working->oscillation_count;
      } else {
        working->reached_depth = true;
        working->min_reached_ms = now_ms;
      }

      working->last_reversal_ms = now_ms;
    }

    if (!working->reached_depth) {
      working->descent_peak_speed = maxf_local(working->descent_peak_speed, maxf_local(0.0f, -velocity));
      if ((working->start_norm - working->min_norm) >= config->step_range_threshold &&
          motion_sign == 1 && tracker->filtered_norm <= working->min_norm + 0.05f) {
        working->reached_depth = true;
        working->min_reached_ms = now_ms;
      }
    } else {
      working->ascent_peak_speed = maxf_local(working->ascent_peak_speed, maxf_local(0.0f, velocity));
    }

    if (motion_sign != 0) {
      working->last_motion_sign = motion_sign;
    }

    if (working->reached_depth &&
        tracker->filtered_norm >= (working->start_norm - config->return_margin) &&
        motion_sign >= 0) {
      bool saved = finish_step(working,
                               live,
                               history,
                               now_ms,
                               timezone_offset_minutes,
                               current_epoch_ms_fn,
                               time_context,
                               config);
      tracker->previous_norm = tracker->filtered_norm;
      tracker->previous_velocity = velocity;
      return saved;
    }

    if ((now_ms - working->started_ms) > config->max_step_duration_ms ||
        (!working->reached_depth && motion_sign == 0 && (now_ms - working->started_ms) > 1200)) {
      memset(working, 0, sizeof(*working));
      live->in_step = false;
    }
  }

  tracker->previous_norm = tracker->filtered_norm;
  tracker->previous_velocity = velocity;
  return false;
}
