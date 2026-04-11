#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <stdlib.h>

#include "generated/web_runtime_generated.h"
#include "json.h"
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

String json_string_from_root(json_data *root) {
  char *json_text;
  String out;

  if (root == nullptr) {
    return "{}";
  }

  json_text = json_tostring(root);
  if (json_text != nullptr) {
    out = String(json_text);
    free(json_text);
  } else {
    out = "{}";
  }

  json_free(root);
  return out;
}

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
  json_data *root = init_json_object();

  if (root == nullptr) {
    return "{}";
  }

  json_object_add_number(root, "reading", live.raw_reading);
  json_object_add_number(root, "percentStraight", live.percent_straight);
  json_object_add_number(root, "angleDeg", live.knee_angle_deg);
  json_object_add_number(root, "speed", live.speed_percent_per_sec);
  json_object_add_number(root, "accel", live.accel_percent_per_sec2);
  json_object_add_boolean(root, "inStep", live.in_step);
  json_object_add_number(root, "lastScore", live.last_score);
  json_object_add_number(root, "stepCount", live.step_count);
  json_object_add_number(
      root,
      "todayAverage",
      rehab_today_average_score(&storage, time_synced, current_epoch_ms(nullptr), timezone_offset_minutes));
  json_object_add_number(root, "goal", DAILY_GOAL_SCORE);
  json_object_add_boolean(root, "timeSynced", time_synced);
  return json_string_from_root(root);
}

String build_variables_json() {
  json_data *root = init_json_object();

  if (root == nullptr) {
    return "{}";
  }

  json_object_add_number(root, "minReading", rehab_config.min_reading);
  json_object_add_number(root, "maxReading", rehab_config.max_reading);
  json_object_add_number(root, "bentAngle", rehab_config.bent_angle);
  json_object_add_number(root, "straightAngle", rehab_config.straight_angle);
  json_object_add_number(root, "sampleIntervalMs", rehab_config.sample_interval_ms);
  json_object_add_number(root, "filterAlpha", rehab_config.filter_alpha);
  json_object_add_number(root, "motionThreshold", rehab_config.motion_threshold);
  json_object_add_number(root, "stepRangeThreshold", rehab_config.step_range_threshold);
  json_object_add_number(root, "startReadyThreshold", rehab_config.start_ready_threshold);
  json_object_add_number(root, "returnMargin", rehab_config.return_margin);
  json_object_add_number(root, "maxStepDurationMs", rehab_config.max_step_duration_ms);
  json_object_add_boolean(root, "sampleHistoryEnabled", ENABLE_SAMPLE_HISTORY);
  return json_string_from_root(root);
}

String build_history_json() {
  DaySummary summaries[14] = {};
  size_t summary_count = 0;
  json_data *root = init_json_object();
  json_data *history = init_json_array(0);
  json_data *daily_averages = init_json_array(0);
  size_t i;

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

  if (root == nullptr || history == nullptr || daily_averages == nullptr) {
    json_free(root);
    json_free(history);
    json_free(daily_averages);
    return "{}";
  }

  json_object_add_number(root, "goal", DAILY_GOAL_SCORE);
  for (int history_index = static_cast<int>(storage.count) - 1; history_index >= 0; --history_index) {
    const StepRecord &record = storage.records[history_index];
    json_data *entry = init_json_object();
    if (entry == nullptr) {
      continue;
    }

    json_object_add_number(entry, "id", record.id);
    json_object_add_number(entry, "timestampMs", static_cast<double>(record.timestamp_ms));
    json_object_add_number(entry, "score", record.score);
    json_object_add_number(entry, "shakiness", record.shakiness_penalty);
    json_object_add_number(entry, "uncontrolledDescent", record.descent_penalty);
    json_object_add_number(entry, "compensation", record.compensation_penalty);
    json_object_add_number(entry, "durationMs", record.duration_ms);
    json_object_add_number(entry, "descentMs", record.descent_ms);
    json_object_add_number(entry, "ascentMs", record.ascent_ms);
    json_object_add_number(entry, "range", record.range);
    json_object_add_number(entry, "startPercent", record.start_percent);
    json_object_add_number(entry, "endPercent", record.end_percent);
    json_object_add_number(entry, "minPercent", record.min_percent);
    json_object_add_number(entry, "maxPercent", record.max_percent);
    json_object_add_number(entry, "descentAvgSpeed", record.descent_avg_speed);
    json_object_add_number(entry, "ascentAvgSpeed", record.ascent_avg_speed);
    json_object_add_number(entry, "descentPeakSpeed", record.descent_peak_speed);
    json_object_add_number(entry, "ascentPeakSpeed", record.ascent_peak_speed);
    json_object_add_number(entry, "oscillations", record.oscillation_count);
    json_array_add_object(history, entry);
  }

  for (i = 0; i < summary_count; ++i) {
    json_data *summary = init_json_object();
    const int64_t day_start_utc_ms =
        summaries[i].key * 86400000LL + static_cast<int64_t>(timezone_offset_minutes) * 60000LL;
    const float average =
        summaries[i].total / static_cast<float>(summaries[i].count > 0 ? summaries[i].count : 1);
    if (summary == nullptr) {
      continue;
    }

    json_object_add_number(summary, "dayStartMs", static_cast<double>(day_start_utc_ms));
    json_object_add_number(summary, "averageScore", average);
    json_object_add_number(summary, "count", summaries[i].count);
    json_array_add_object(daily_averages, summary);
  }

  json_object_add_array(root, "history", history);
  json_object_add_array(root, "dailyAverages", daily_averages);
  return json_string_from_root(root);
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
    json_data *root = init_json_object();
    if (root != nullptr) {
      json_object_add_boolean(root, "ok", false);
      json_object_add_string(root, "message", "Invalid variable values");
    }
    server.send(400, "application/json", json_string_from_root(root));
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

  json_data *root = init_json_object();
  if (root != nullptr) {
    json_object_add_boolean(root, "ok", true);
  }
  server.send(200, "application/json", json_string_from_root(root));
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
