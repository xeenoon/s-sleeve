#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

#include "generated/web_runtime_generated.h"
#include "rehab_metrics.h"
#include "rehab_types.h"
#include "sample_history.h"

namespace {

constexpr int POT_PIN = 34;
constexpr char AP_SSID[] = "ESP32-Pot-Reader";
constexpr char PREF_NAMESPACE[] = "rehab";
constexpr char PREF_KEY_HISTORY[] = "history";
constexpr char PREF_KEY_VARS[] = "vars";
constexpr uint32_t VARIABLES_MAGIC = 0x56415253UL;

struct PersistedVariables {
  uint32_t magic;
  RehabConfig config;
};

struct DaySummary {
  int64_t key;
  float total;
  uint16_t count;
};

RehabConfig rehab_config = {
    3100,
    3700,
    78.0f,
    0.0f,
    40,
    0.24f,
    0.16f,
    0.18f,
    0.58f,
    0.08f,
    5000};
PersistedVariables persisted_variables = {VARIABLES_MAGIC, rehab_config};

WebServer server(80);
Preferences preferences;
PersistedHistory storage = {};
LiveMetrics live = {};
WorkingStep working = {};
RehabTrackerState tracker = {};

bool time_synced = false;
int timezone_offset_minutes = 0;
int64_t epoch_offset_ms = 0;

void save_history();
void save_variables();

uint64_t current_epoch_ms(void *) {
  if (!time_synced) {
    return 0;
  }

  return static_cast<uint64_t>(epoch_offset_ms + static_cast<int64_t>(millis()));
}

void refresh_live_summary() {
  live.step_count = storage.count;
  live.last_score = storage.count > 0 ? storage.records[storage.count - 1].score : 0.0f;
  live.time_synced = time_synced;
}

String build_live_json() {
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
  json += String(rehab_today_average_score(&storage, time_synced, current_epoch_ms(nullptr), timezone_offset_minutes), 1);
  json += ",\"goal\":";
  json += String(DAILY_GOAL_SCORE);
  json += ",\"timeSynced\":";
  json += time_synced ? "true" : "false";
  json += "}";
  return json;
}

String build_variables_json() {
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

String build_history_json() {
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
    const int64_t day_start_utc_ms =
        summaries[i].key * 86400000LL + static_cast<int64_t>(timezone_offset_minutes) * 60000LL;
    const float average =
        summaries[i].total / static_cast<float>(summaries[i].count > 0 ? summaries[i].count : 1);
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

void load_variables() {
  preferences.begin(PREF_NAMESPACE, true);
  const size_t bytes = preferences.getBytesLength(PREF_KEY_VARS);
  if (bytes == sizeof(persisted_variables)) {
    preferences.getBytes(PREF_KEY_VARS, &persisted_variables, sizeof(persisted_variables));
  }
  preferences.end();

  if (persisted_variables.magic != VARIABLES_MAGIC) {
    persisted_variables.magic = VARIABLES_MAGIC;
    persisted_variables.config = rehab_config;
    save_variables();
  } else {
    rehab_config = persisted_variables.config;
  }
}

void load_history() {
  bool had_records = false;

  preferences.begin(PREF_NAMESPACE, true);
  const size_t bytes = preferences.getBytesLength(PREF_KEY_HISTORY);
  if (bytes == sizeof(storage)) {
    preferences.getBytes(PREF_KEY_HISTORY, &storage, sizeof(storage));
  }
  preferences.end();

  if (storage.magic != STORAGE_MAGIC || storage.count > MAX_HISTORY || storage.next_id == 0) {
    memset(&storage, 0, sizeof(storage));
    storage.magic = STORAGE_MAGIC;
    storage.next_id = 1;
  }

  had_records = storage.count > 0;
  seed_sample_history(&storage);
  refresh_live_summary();

  if (!had_records && storage.count > 0) {
    save_history();
  }
}

void save_history() {
  storage.magic = STORAGE_MAGIC;
  preferences.begin(PREF_NAMESPACE, false);
  preferences.putBytes(PREF_KEY_HISTORY, &storage, sizeof(storage));
  preferences.end();
}

void save_variables() {
  persisted_variables.magic = VARIABLES_MAGIC;
  persisted_variables.config = rehab_config;
  preferences.begin(PREF_NAMESPACE, false);
  preferences.putBytes(PREF_KEY_VARS, &persisted_variables, sizeof(persisted_variables));
  preferences.end();
}

void handle_root() {
  generated_web_send_root(server);
}

void handle_styles() {
  generated_web_send_styles(server);
}

void handle_app_js() {
  generated_web_send_app_js(server);
}

void handle_value() {
  server.send(200, "text/plain", String(live.raw_reading));
}

void handle_live() {
  server.send(200, "application/json", build_live_json());
}

void handle_history() {
  server.send(200, "application/json", build_history_json());
}

void handle_variables() {
  server.send(200, "application/json", build_variables_json());
}

bool update_int_arg(const char *name, int *value) {
  if (!server.hasArg(name)) {
    return false;
  }
  *value = server.arg(name).toInt();
  return true;
}

bool update_float_arg(const char *name, float *value) {
  if (!server.hasArg(name)) {
    return false;
  }
  *value = server.arg(name).toFloat();
  return true;
}

void handle_variables_update() {
  RehabConfig updated = rehab_config;

  update_int_arg("minReading", &updated.min_reading);
  update_int_arg("maxReading", &updated.max_reading);
  update_float_arg("bentAngle", &updated.bent_angle);
  update_float_arg("straightAngle", &updated.straight_angle);
  if (server.hasArg("sampleIntervalMs")) {
    updated.sample_interval_ms = static_cast<unsigned long>(server.arg("sampleIntervalMs").toInt());
  }
  update_float_arg("filterAlpha", &updated.filter_alpha);
  update_float_arg("motionThreshold", &updated.motion_threshold);
  update_float_arg("stepRangeThreshold", &updated.step_range_threshold);
  update_float_arg("startReadyThreshold", &updated.start_ready_threshold);
  update_float_arg("returnMargin", &updated.return_margin);
  if (server.hasArg("maxStepDurationMs")) {
    updated.max_step_duration_ms = static_cast<unsigned long>(server.arg("maxStepDurationMs").toInt());
  }

  if (updated.max_reading <= updated.min_reading ||
      updated.sample_interval_ms < 10 ||
      updated.filter_alpha <= 0.0f || updated.filter_alpha > 1.0f ||
      updated.motion_threshold <= 0.0f ||
      updated.step_range_threshold <= 0.0f ||
      updated.start_ready_threshold < 0.0f || updated.start_ready_threshold > 1.0f ||
      updated.return_margin < 0.0f || updated.return_margin > 1.0f ||
      updated.max_step_duration_ms < 250) {
    server.send(400, "application/json", "{\"ok\":false,\"message\":\"Invalid variable values\"}");
    return;
  }

  rehab_config = updated;
  const int current_reading = analogRead(POT_PIN);
  rehab_tracker_init(&tracker, current_reading, &rehab_config);
  live.raw_reading = current_reading;
  live.normalized = tracker.filtered_norm;
  live.knee_angle_deg = rehab_normalized_to_angle(tracker.filtered_norm, &rehab_config);
  live.percent_straight = tracker.filtered_norm * 100.0f;
  memset(&working, 0, sizeof(working));
  live.in_step = false;
  save_variables();

  server.send(200, "application/json", build_variables_json());
}

void handle_time_sync() {
  if (server.hasArg("epochMs")) {
    epoch_offset_ms = atoll(server.arg("epochMs").c_str()) - static_cast<int64_t>(millis());
    time_synced = true;
    live.time_synced = true;
  }

  if (server.hasArg("offsetMinutes")) {
    timezone_offset_minutes = server.arg("offsetMinutes").toInt();
  }

  server.send(200, "application/json", "{\"ok\":true}");
}

void handle_not_found() {
  if (generated_web_try_send_static_route(server, server.uri(), server.method())) {
    return;
  }
  server.send(404, "text/plain", "Not found");
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);

  analogReadResolution(12);
  pinMode(POT_PIN, INPUT);

  load_variables();
  load_history();

  const int initial_reading = analogRead(POT_PIN);
  rehab_tracker_init(&tracker, initial_reading, &rehab_config);

  live.raw_reading = initial_reading;
  live.normalized = tracker.filtered_norm;
  live.knee_angle_deg = rehab_normalized_to_angle(tracker.filtered_norm, &rehab_config);
  live.percent_straight = tracker.filtered_norm * 100.0f;
  refresh_live_summary();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);

  server.on("/", handle_root);
  server.on("/styles.css", handle_styles);
  server.on("/app.js", handle_app_js);
  server.on("/value", handle_value);
  server.on("/api/live", handle_live);
  server.on("/api/history", handle_history);
  server.on("/api/variables", HTTP_GET, handle_variables);
  server.on("/api/variables", HTTP_POST, handle_variables_update);
  server.on("/api/time-sync", handle_time_sync);
  server.onNotFound(handle_not_found);
  server.begin();

  Serial.println();
  Serial.println("ESP32 knee rehab tracker started.");
  Serial.print("Connect to Wi-Fi: ");
  Serial.println(AP_SSID);
  Serial.print("Open: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  if (rehab_update_sensor_processing(&tracker,
                                     &working,
                                     &live,
                                     &storage,
                                     analogRead(POT_PIN),
                                     millis(),
                                     time_synced,
                                     timezone_offset_minutes,
                                     current_epoch_ms,
                                     nullptr,
                                     &rehab_config)) {
    save_history();
  }

  server.handleClient();
}
