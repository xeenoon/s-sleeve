#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdlib.h>

#include "generated/web_runtime_generated.h"
#include "json.h"
#include "rehab_metrics.h"
#include "rehab_types.h"
#include "sample_history.h"

namespace {

constexpr int POT_PIN = 34;
constexpr char AP_SSID[] = "s-sleeve";
constexpr char SITE_HOSTNAME[] = "s-sleeve.com";
constexpr char PREF_NAMESPACE[] = "rehab";
constexpr char PREF_KEY_HISTORY[] = "history";
constexpr char PREF_KEY_VARS[] = "vars";
constexpr uint32_t VARIABLES_MAGIC = 0x56415253UL;
constexpr uint32_t SENSOR_TASK_PERIOD_MS = 2;
constexpr uint32_t SENSOR_TASK_STACK_BYTES = 6144;

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
    10,
    0.24f,
    0.16f,
    0.18f,
    0.58f,
    0.08f,
    5000};
PersistedVariables persisted_variables = {VARIABLES_MAGIC, rehab_config};

WebServer server(80);
DNSServer dns_server;
Preferences preferences;
PersistedHistory storage = {};
LiveMetrics live = {};
WorkingStep working = {};
RehabTrackerState tracker = {};
PersistedHistory history_save_snapshot_buffer = {};
PersistedHistory web_history_snapshot = {};
LiveMetrics web_live_snapshot = {};
RehabConfig web_config_snapshot = {};
TaskHandle_t sensor_task_handle = nullptr;
portMUX_TYPE state_mux = portMUX_INITIALIZER_UNLOCKED;

bool time_synced = false;
int timezone_offset_minutes = 0;
int64_t epoch_offset_ms = 0;
bool web_time_synced_snapshot = false;
int web_timezone_offset_snapshot = 0;
int64_t web_epoch_offset_snapshot = 0;
bool history_save_pending = false;

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

struct TimeSnapshot {
  bool synced;
  int64_t epoch_offset;
};

uint64_t current_epoch_ms_from_snapshot(void *context) {
  const TimeSnapshot *snapshot = static_cast<const TimeSnapshot *>(context);
  if (snapshot == nullptr || !snapshot->synced) {
    return 0;
  }

  return static_cast<uint64_t>(snapshot->epoch_offset + static_cast<int64_t>(millis()));
}

uint64_t current_epoch_ms_with_values(bool synced, int64_t epoch_offset) {
  if (!synced) {
    return 0;
  }

  return static_cast<uint64_t>(epoch_offset + static_cast<int64_t>(millis()));
}

void refresh_live_summary_locked() {
  live.step_count = storage.count;
  live.last_score = storage.count > 0 ? storage.records[storage.count - 1].score : 0.0f;
  live.time_synced = time_synced;
}

void copy_live_snapshot(LiveMetrics *live_out, bool *time_synced_out, int64_t *epoch_offset_out) {
  portENTER_CRITICAL(&state_mux);
  *live_out = live;
  *time_synced_out = time_synced;
  *epoch_offset_out = epoch_offset_ms;
  portEXIT_CRITICAL(&state_mux);
}

void copy_history_snapshot(PersistedHistory *history_out, bool *time_synced_out, int *timezone_offset_out, int64_t *epoch_offset_out) {
  portENTER_CRITICAL(&state_mux);
  *history_out = storage;
  *time_synced_out = time_synced;
  *timezone_offset_out = timezone_offset_minutes;
  *epoch_offset_out = epoch_offset_ms;
  portEXIT_CRITICAL(&state_mux);
}

void copy_variables_snapshot(RehabConfig *config_out) {
  portENTER_CRITICAL(&state_mux);
  *config_out = rehab_config;
  portEXIT_CRITICAL(&state_mux);
}

bool take_history_save_snapshot() {
  bool should_save;

  portENTER_CRITICAL(&state_mux);
  should_save = history_save_pending;
  if (should_save) {
    history_save_snapshot_buffer = storage;
    history_save_pending = false;
  }
  portEXIT_CRITICAL(&state_mux);

  return should_save;
}

String build_motion_json(const LiveMetrics &live_snapshot, bool time_synced_snapshot) {
  json_data *root = init_json_object();

  if (root == nullptr) {
    return "{}";
  }

  json_object_add_number(root, "reading", live_snapshot.raw_reading);
  json_object_add_number(root, "percentStraight", live_snapshot.percent_straight);
  json_object_add_number(root, "angleDeg", live_snapshot.knee_angle_deg);
  json_object_add_number(root, "speed", live_snapshot.speed_percent_per_sec);
  json_object_add_boolean(root, "inStep", live_snapshot.in_step);
  json_object_add_boolean(root, "timeSynced", time_synced_snapshot);
  return json_string_from_root(root);
}

String build_live_json(const LiveMetrics &live_snapshot,
                       const PersistedHistory &history_snapshot,
                       bool time_synced_snapshot,
                       int timezone_offset_snapshot,
                       int64_t epoch_offset_snapshot) {
  json_data *root = init_json_object();

  if (root == nullptr) {
    return "{}";
  }

  json_object_add_number(root, "reading", live_snapshot.raw_reading);
  json_object_add_number(root, "percentStraight", live_snapshot.percent_straight);
  json_object_add_number(root, "angleDeg", live_snapshot.knee_angle_deg);
  json_object_add_number(root, "speed", live_snapshot.speed_percent_per_sec);
  json_object_add_number(root, "accel", live_snapshot.accel_percent_per_sec2);
  json_object_add_boolean(root, "inStep", live_snapshot.in_step);
  json_object_add_number(root, "lastScore", live_snapshot.last_score);
  json_object_add_number(root, "stepCount", live_snapshot.step_count);
  json_object_add_number(
      root,
      "todayAverage",
      rehab_today_average_score(
          &history_snapshot,
          time_synced_snapshot,
          current_epoch_ms_with_values(time_synced_snapshot, epoch_offset_snapshot),
          timezone_offset_snapshot));
  json_object_add_number(root, "goal", DAILY_GOAL_SCORE);
  json_object_add_boolean(root, "timeSynced", time_synced_snapshot);
  return json_string_from_root(root);
}

String build_variables_json(const RehabConfig &config_snapshot) {
  json_data *root = init_json_object();

  if (root == nullptr) {
    return "{}";
  }

  json_object_add_number(root, "minReading", config_snapshot.min_reading);
  json_object_add_number(root, "maxReading", config_snapshot.max_reading);
  json_object_add_number(root, "bentAngle", config_snapshot.bent_angle);
  json_object_add_number(root, "straightAngle", config_snapshot.straight_angle);
  json_object_add_number(root, "sampleIntervalMs", config_snapshot.sample_interval_ms);
  json_object_add_number(root, "filterAlpha", config_snapshot.filter_alpha);
  json_object_add_number(root, "motionThreshold", config_snapshot.motion_threshold);
  json_object_add_number(root, "stepRangeThreshold", config_snapshot.step_range_threshold);
  json_object_add_number(root, "startReadyThreshold", config_snapshot.start_ready_threshold);
  json_object_add_number(root, "returnMargin", config_snapshot.return_margin);
  json_object_add_number(root, "maxStepDurationMs", config_snapshot.max_step_duration_ms);
  json_object_add_boolean(root, "sampleHistoryEnabled", ENABLE_SAMPLE_HISTORY);
  return json_string_from_root(root);
}

String build_history_json(const PersistedHistory &history_snapshot, int timezone_offset_snapshot) {
  DaySummary summaries[14] = {};
  size_t summary_count = 0;
  json_data *root = init_json_object();
  json_data *history = init_json_array(0);
  json_data *daily_averages = init_json_array(0);
  size_t i;

  for (int i = static_cast<int>(history_snapshot.count) - 1; i >= 0; --i) {
    const int64_t key = rehab_local_day_key(history_snapshot.records[i].timestamp_ms, timezone_offset_snapshot);
    if (key < 0) {
      continue;
    }
    bool found = false;
    for (size_t s = 0; s < summary_count; ++s) {
      if (summaries[s].key == key) {
        summaries[s].total += history_snapshot.records[i].score;
        ++summaries[s].count;
        found = true;
        break;
      }
    }
    if (!found && summary_count < 14) {
      summaries[summary_count].key = key;
      summaries[summary_count].total = history_snapshot.records[i].score;
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
  for (int history_index = static_cast<int>(history_snapshot.count) - 1; history_index >= 0; --history_index) {
    const StepRecord &record = history_snapshot.records[history_index];
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
        summaries[i].key * 86400000LL + static_cast<int64_t>(timezone_offset_snapshot) * 60000LL;
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
  refresh_live_summary_locked();

  if (!had_records && storage.count > 0) {
    save_history();
  }
}

void save_history_snapshot(const PersistedHistory &history_snapshot) {
  preferences.begin(PREF_NAMESPACE, false);
  preferences.putBytes(PREF_KEY_HISTORY, &history_snapshot, sizeof(history_snapshot));
  preferences.end();
}

void save_history() {
  portENTER_CRITICAL(&state_mux);
  history_save_snapshot_buffer = storage;
  portEXIT_CRITICAL(&state_mux);

  save_history_snapshot(history_save_snapshot_buffer);
}

void save_variables() {
  PersistedVariables snapshot = {};

  portENTER_CRITICAL(&state_mux);
  persisted_variables.magic = VARIABLES_MAGIC;
  persisted_variables.config = rehab_config;
  snapshot = persisted_variables;
  portEXIT_CRITICAL(&state_mux);

  preferences.begin(PREF_NAMESPACE, false);
  preferences.putBytes(PREF_KEY_VARS, &snapshot, sizeof(snapshot));
  preferences.end();
}

void handle_root() {
  generated_web_send_root(server);
}

String captive_root_url() {
  return String("http://") + SITE_HOSTNAME + "/";
}

void handle_styles() {
  generated_web_send_styles(server);
}

void handle_app_js() {
  generated_web_send_app_js(server);
}

void handle_value() {
  LiveMetrics live_snapshot = {};
  bool time_synced_snapshot = false;
  int64_t epoch_offset_snapshot = 0;

  copy_live_snapshot(&live_snapshot, &time_synced_snapshot, &epoch_offset_snapshot);
  (void)time_synced_snapshot;
  (void)epoch_offset_snapshot;
  server.send(200, "text/plain", String(live_snapshot.raw_reading));
}

void handle_motion() {
  copy_live_snapshot(&web_live_snapshot, &web_time_synced_snapshot, &web_epoch_offset_snapshot);
  (void)web_epoch_offset_snapshot;
  server.send(200, "application/json", build_motion_json(web_live_snapshot, web_time_synced_snapshot));
}

void handle_live() {
  portENTER_CRITICAL(&state_mux);
  web_live_snapshot = live;
  web_history_snapshot = storage;
  web_time_synced_snapshot = time_synced;
  web_timezone_offset_snapshot = timezone_offset_minutes;
  web_epoch_offset_snapshot = epoch_offset_ms;
  portEXIT_CRITICAL(&state_mux);

  server.send(
      200,
      "application/json",
      build_live_json(
          web_live_snapshot,
          web_history_snapshot,
          web_time_synced_snapshot,
          web_timezone_offset_snapshot,
          web_epoch_offset_snapshot));
}

void handle_history() {
  copy_history_snapshot(
      &web_history_snapshot, &web_time_synced_snapshot, &web_timezone_offset_snapshot, &web_epoch_offset_snapshot);
  (void)web_time_synced_snapshot;
  (void)web_epoch_offset_snapshot;
  server.send(200, "application/json", build_history_json(web_history_snapshot, web_timezone_offset_snapshot));
}

void handle_variables() {
  copy_variables_snapshot(&web_config_snapshot);
  server.send(200, "application/json", build_variables_json(web_config_snapshot));
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
  RehabConfig updated = {};

  copy_variables_snapshot(&updated);

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

  const int current_reading = analogRead(POT_PIN);
  portENTER_CRITICAL(&state_mux);
  rehab_config = updated;
  rehab_tracker_init(&tracker, current_reading, &rehab_config);
  live.raw_reading = current_reading;
  live.normalized = tracker.filtered_norm;
  live.knee_angle_deg = rehab_normalized_to_angle(tracker.filtered_norm, &rehab_config);
  live.percent_straight = tracker.filtered_norm * 100.0f;
  live.speed_percent_per_sec = 0.0f;
  live.accel_percent_per_sec2 = 0.0f;
  memset(&working, 0, sizeof(working));
  live.in_step = false;
  refresh_live_summary_locked();
  portEXIT_CRITICAL(&state_mux);
  save_variables();

  server.send(200, "application/json", build_variables_json(updated));
}

void handle_time_sync() {
  bool sync_enabled = false;
  int64_t next_epoch_offset_ms = 0;
  int next_timezone_offset_minutes = 0;

  portENTER_CRITICAL(&state_mux);
  sync_enabled = time_synced;
  next_epoch_offset_ms = epoch_offset_ms;
  next_timezone_offset_minutes = timezone_offset_minutes;
  portEXIT_CRITICAL(&state_mux);

  if (server.hasArg("epochMs")) {
    next_epoch_offset_ms = atoll(server.arg("epochMs").c_str()) - static_cast<int64_t>(millis());
    sync_enabled = true;
  }

  if (server.hasArg("offsetMinutes")) {
    next_timezone_offset_minutes = server.arg("offsetMinutes").toInt();
  }

  portENTER_CRITICAL(&state_mux);
  epoch_offset_ms = next_epoch_offset_ms;
  time_synced = sync_enabled;
  timezone_offset_minutes = next_timezone_offset_minutes;
  live.time_synced = sync_enabled;
  portEXIT_CRITICAL(&state_mux);

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
  if (server.method() == HTTP_GET) {
    Serial.print("[http] captive redirect uri=");
    Serial.print(server.uri());
    Serial.print(" host=");
    Serial.println(server.hostHeader());
    server.sendHeader("Location", captive_root_url(), true);
    server.send(302, "text/plain", "Redirecting to s-sleeve");
    return;
  }
  server.send(404, "text/plain", "Not found");
}

void sensor_task(void *) {
  TickType_t last_wake = xTaskGetTickCount();

  for (;;) {
    const int raw_reading = analogRead(POT_PIN);
    const unsigned long now_ms = millis();
    TimeSnapshot time_snapshot = {};
    int timezone_offset_snapshot = 0;
    bool completed_step = false;

    portENTER_CRITICAL(&state_mux);
    time_snapshot.synced = time_synced;
    time_snapshot.epoch_offset = epoch_offset_ms;
    timezone_offset_snapshot = timezone_offset_minutes;
    completed_step = rehab_update_sensor_processing(&tracker,
                                                    &working,
                                                    &live,
                                                    &storage,
                                                    raw_reading,
                                                    now_ms,
                                                    time_snapshot.synced,
                                                    timezone_offset_snapshot,
                                                    current_epoch_ms_from_snapshot,
                                                    &time_snapshot,
                                                    &rehab_config);
    if (completed_step) {
      history_save_pending = true;
    }
    refresh_live_summary_locked();
    portEXIT_CRITICAL(&state_mux);

    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SENSOR_TASK_PERIOD_MS));
  }
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
  refresh_live_summary_locked();

  Serial.println("[boot] configuring WiFi AP mode");
  WiFi.mode(WIFI_AP);
  Serial.println("[boot] starting softAP");
  WiFi.softAP(AP_SSID);
  Serial.print("[boot] softAP started, status=");
  Serial.println(static_cast<int>(WiFi.status()));
  Serial.print("[boot] softAP IP=");
  Serial.println(WiFi.softAPIP());
  Serial.println("[boot] starting captive DNS");
  dns_server.start(53, "*", WiFi.softAPIP());
  Serial.print("[boot] captive DNS ready for host ");
  Serial.println(SITE_HOSTNAME);

  server.on("/", handle_root);
  server.on("/styles.css", handle_styles);
  server.on("/app.js", handle_app_js);
  server.on("/value", handle_value);
  server.on("/api/motion", handle_motion);
  server.on("/api/live", handle_live);
  server.on("/api/history", handle_history);
  server.on("/api/variables", HTTP_GET, handle_variables);
  server.on("/api/variables", HTTP_POST, handle_variables_update);
  server.on("/api/time-sync", handle_time_sync);
  server.onNotFound(handle_not_found);
  Serial.println("[boot] starting web server");
  server.begin();
  Serial.println("[boot] web server started");

  Serial.println();
  Serial.println("ESP32 knee rehab tracker started.");
  Serial.print("Connect to Wi-Fi: ");
  Serial.println(AP_SSID);
  Serial.print("Open: http://");
  Serial.println(SITE_HOSTNAME);
  Serial.print("Fallback IP: http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("Sensor task pinned for high-priority motion sampling.");

  xTaskCreatePinnedToCore(
      sensor_task, "sensor_task", SENSOR_TASK_STACK_BYTES, nullptr, 3, &sensor_task_handle, 1);
}

void loop() {
  if (take_history_save_snapshot()) {
    save_history_snapshot(history_save_snapshot_buffer);
  }

  dns_server.processNextRequest();
  server.handleClient();
  delay(1);
}
