#include <Arduino.h>
#include <WebServer.h>

#include <string.h>

#include "generated/web_runtime_generated.h"
#include "web_page.h"

namespace {
struct DaySummary {
  int64_t key;
  float total;
  uint16_t count;
};
}  // namespace

void generated_web_send_root(WebServer &server) {
  server.send_P(200, "text/html", INDEX_HTML);
}

void generated_web_send_styles(WebServer &server) {
  server.send_P(200, "text/css", STYLES_CSS);
}

void generated_web_send_app_js(WebServer &server) {
  server.send_P(200, "application/javascript", APP_JS);
}

void generated_web_send_variables_page(WebServer &server) {
  server.send_P(200, "text/html", INDEX_HTML);
}

String generated_web_build_live_json(const LiveMetrics &live,
                                     const PersistedHistory &storage,
                                     bool time_synced,
                                     uint64_t current_epoch_ms,
                                     int timezone_offset_minutes) {
  String json;
  json.reserve(320);
  json += "{";
  json += "\"reading\":";
  json += String(live.raw_reading);
  json += ",\"percentStraight\":";
  json += String(live.percent_straight, 1);
  json += ",\"angleDeg\":";
  json += String(live.knee_angle_deg, 1);
  json += ",\"speed\":";
  json += String(live.speed_percent_per_sec, 1);
  json += ",\"accel\":";
  json += String(live.accel_percent_per_sec2, 1);
  json += ",\"inStep\":";
  json += live.in_step ? "true" : "false";
  json += ",\"lastScore\":";
  json += String(live.last_score, 1);
  json += ",\"stepCount\":";
  json += String(live.step_count);
  json += ",\"todayAverage\":";
  json += String(rehab_today_average_score(&storage, time_synced, current_epoch_ms, timezone_offset_minutes), 1);
  json += ",\"goal\":";
  json += String(DAILY_GOAL_SCORE);
  json += ",\"timeSynced\":";
  json += time_synced ? "true" : "false";
  json += "}";
  return json;
}

String generated_web_build_variables_json(const RehabConfig &rehab_config) {
  String json;
  json.reserve(480);
  json += "{";
  json += "\"minReading\":";
  json += String(rehab_config.min_reading);
  json += ",\"maxReading\":";
  json += String(rehab_config.max_reading);
  json += ",\"bentAngle\":";
  json += String(rehab_config.bent_angle, 2);
  json += ",\"straightAngle\":";
  json += String(rehab_config.straight_angle, 2);
  json += ",\"sampleIntervalMs\":";
  json += String(rehab_config.sample_interval_ms);
  json += ",\"filterAlpha\":";
  json += String(rehab_config.filter_alpha, 3);
  json += ",\"motionThreshold\":";
  json += String(rehab_config.motion_threshold, 3);
  json += ",\"stepRangeThreshold\":";
  json += String(rehab_config.step_range_threshold, 3);
  json += ",\"startReadyThreshold\":";
  json += String(rehab_config.start_ready_threshold, 3);
  json += ",\"returnMargin\":";
  json += String(rehab_config.return_margin, 3);
  json += ",\"maxStepDurationMs\":";
  json += String(rehab_config.max_step_duration_ms);
  json += ",\"sampleHistoryEnabled\":";
  json += ENABLE_SAMPLE_HISTORY ? "true" : "false";
  json += "}";
  return json;
}

String generated_web_build_history_json(const PersistedHistory &storage,
                                        int timezone_offset_minutes) {
  DaySummary summaries[14] = {};
  size_t summary_count = 0;
  String json;
  for (int i = static_cast<int>(storage.count) - 1; i >= 0; --i) {
    const int64_t key = rehab_local_day_key(storage.records[i].timestamp_ms, timezone_offset_minutes);
    if (key < 0) {
      continue;
    }
    bool found = false;
    for (size_t s = 0; s < summary_count; ++s) {
      if (summaries[s].key == key) {
        summaries[s].total += storage.records[i].score;
        ++summaries[s].count;
        found = true;
        break;
      }
    }
    if (!found && summary_count < 14) {
      summaries[summary_count].key = key;
      summaries[summary_count].total = storage.records[i].score;
      summaries[summary_count].count = 1;
      ++summary_count;
    }
  }
  json.reserve(22000);
  json += "{";
  json += "\"goal\":";
  json += String(DAILY_GOAL_SCORE);
  json += ",\"history\":[";
  for (int i = static_cast<int>(storage.count) - 1; i >= 0; --i) {
    const StepRecord &record = storage.records[i];
    if (i != static_cast<int>(storage.count) - 1) {
      json += ",";
    }
    json += "{";
    json += "\"id\":";
    json += String(record.id);
    json += ",\"timestampMs\":";
    json += String(static_cast<unsigned long long>(record.timestamp_ms));
    json += ",\"score\":";
    json += String(record.score, 1);
    json += ",\"shakiness\":";
    json += String(record.shakiness_penalty, 1);
    json += ",\"uncontrolledDescent\":";
    json += String(record.descent_penalty, 1);
    json += ",\"compensation\":";
    json += String(record.compensation_penalty, 1);
    json += ",\"durationMs\":";
    json += String(record.duration_ms, 0);
    json += ",\"descentMs\":";
    json += String(record.descent_ms, 0);
    json += ",\"ascentMs\":";
    json += String(record.ascent_ms, 0);
    json += ",\"range\":";
    json += String(record.range, 1);
    json += ",\"startPercent\":";
    json += String(record.start_percent, 1);
    json += ",\"endPercent\":";
    json += String(record.end_percent, 1);
    json += ",\"minPercent\":";
    json += String(record.min_percent, 1);
    json += ",\"maxPercent\":";
    json += String(record.max_percent, 1);
    json += ",\"descentAvgSpeed\":";
    json += String(record.descent_avg_speed, 1);
    json += ",\"ascentAvgSpeed\":";
    json += String(record.ascent_avg_speed, 1);
    json += ",\"descentPeakSpeed\":";
    json += String(record.descent_peak_speed, 1);
    json += ",\"ascentPeakSpeed\":";
    json += String(record.ascent_peak_speed, 1);
    json += ",\"oscillations\":";
    json += String(record.oscillation_count);
    json += "}";
  }
  json += "],\"dailyAverages\":[";
  for (size_t i = 0; i < summary_count; ++i) {
    const int64_t day_start_utc_ms = summaries[i].key * 86400000LL + static_cast<int64_t>(timezone_offset_minutes) * 60000LL;
    const float average = summaries[i].total / static_cast<float>(summaries[i].count > 0 ? summaries[i].count : 1);
    if (i != 0) {
      json += ",";
    }
    json += "{";
    json += "\"dayStartMs\":";
    json += String(static_cast<long long>(day_start_utc_ms));
    json += ",\"averageScore\":";
    json += String(average, 1);
    json += ",\"count\":";
    json += String(summaries[i].count);
    json += "}";
  }
  json += "]}";
  return json;
}
