#include "generator.h"

#include <stdio.h>
#include <string.h>

#include "asset_writer.h"
#include "file_io.h"
#include "log.h"
#include "output_fs.h"

static const char *g_generated_files[] = {
    "web_runtime_generated.h",
    "web_runtime_generated.cpp",
    "web_page_generated.cpp",
    "index.html",
    "styles.css",
    "app.js"};

static void generator_build_path(char *buffer,
                                 size_t buffer_size,
                                 const char *directory,
                                 const char *filename) {
  snprintf(buffer, buffer_size, "%s\\%s", directory, filename);
  LOG_TRACE("generator_build_path directory=%s filename=%s path=%s\n", directory, filename, buffer);
}

static int generator_write_text_asset(const char *output_dir,
                                      const char *filename,
                                      const char *text) {
  char path[512];

  generator_build_path(path, sizeof(path), output_dir, filename);
  LOG_TRACE("generator_write_text_asset filename=%s bytes=%zu\n",
            filename,
            text != NULL ? strlen(text) : 0u);
  return output_fs_write_text(path, text != NULL ? text : "");
}

static int generator_emit_cpp_bundle(const char *output_dir,
                                     const char *html_source,
                                     const char *css_source,
                                     const char *js_source) {
  char path[512];
  FILE *file;

  generator_build_path(path, sizeof(path), output_dir, "web_page_generated.cpp");
  LOG_TRACE("generator_emit_cpp_bundle path=%s\n", path);

  file = fopen(path, "wb");
  if (file == NULL) {
    log_errorf("failed to open generated bundle for write: %s\n", path);
    return 1;
  }

  if (asset_writer_write_prolog(file) != 0 ||
      asset_writer_write_asset(file, "INDEX_HTML", "HTML_ASSET", html_source) != 0 ||
      asset_writer_write_asset(file, "STYLES_CSS", "CSS_ASSET", css_source) != 0 ||
      asset_writer_write_asset(file, "APP_JS", "JS_ASSET", js_source) != 0 ||
      asset_writer_write_epilog(file) != 0) {
    fclose(file);
    log_errorf("failed to write generated C++ web bundle: %s\n", path);
    return 1;
  }

  fclose(file);
  return 0;
}

static int generator_emit_runtime_header(const char *output_dir) {
  const char *header_text =
      "#ifndef GENERATED_WEB_RUNTIME_H\n"
      "#define GENERATED_WEB_RUNTIME_H\n\n"
      "#include <Arduino.h>\n"
      "#include <WebServer.h>\n\n"
      "#include \"rehab_metrics.h\"\n"
      "#include \"rehab_types.h\"\n\n"
      "void generated_web_send_root(WebServer &server);\n"
      "void generated_web_send_styles(WebServer &server);\n"
      "void generated_web_send_app_js(WebServer &server);\n"
      "void generated_web_send_variables_page(WebServer &server);\n"
      "String generated_web_build_live_json(const LiveMetrics &live,\n"
      "                                     const PersistedHistory &storage,\n"
      "                                     bool time_synced,\n"
      "                                     uint64_t current_epoch_ms,\n"
      "                                     int timezone_offset_minutes);\n"
      "String generated_web_build_variables_json(const RehabConfig &rehab_config);\n"
      "String generated_web_build_history_json(const PersistedHistory &storage,\n"
      "                                        int timezone_offset_minutes);\n\n"
      "#endif\n";

  return generator_write_text_asset(output_dir, "web_runtime_generated.h", header_text);
}

static int generator_emit_runtime_source(const char *output_dir) {
  const char *source_text =
      "#include <Arduino.h>\n"
      "#include <WebServer.h>\n\n"
      "#include <string.h>\n\n"
      "#include \"generated/web_runtime_generated.h\"\n"
      "#include \"web_page.h\"\n\n"
      "namespace {\n"
      "struct DaySummary {\n"
      "  int64_t key;\n"
      "  float total;\n"
      "  uint16_t count;\n"
      "};\n"
      "}  // namespace\n\n"
      "void generated_web_send_root(WebServer &server) {\n"
      "  server.send_P(200, \"text/html\", INDEX_HTML);\n"
      "}\n\n"
      "void generated_web_send_styles(WebServer &server) {\n"
      "  server.send_P(200, \"text/css\", STYLES_CSS);\n"
      "}\n\n"
      "void generated_web_send_app_js(WebServer &server) {\n"
      "  server.send_P(200, \"application/javascript\", APP_JS);\n"
      "}\n\n"
      "void generated_web_send_variables_page(WebServer &server) {\n"
      "  server.send_P(200, \"text/html\", INDEX_HTML);\n"
      "}\n\n"
      "String generated_web_build_live_json(const LiveMetrics &live,\n"
      "                                     const PersistedHistory &storage,\n"
      "                                     bool time_synced,\n"
      "                                     uint64_t current_epoch_ms,\n"
      "                                     int timezone_offset_minutes) {\n"
      "  String json;\n"
      "  json.reserve(320);\n"
      "  json += \"{\";\n"
      "  json += \"\\\"reading\\\":\";\n"
      "  json += String(live.raw_reading);\n"
      "  json += \",\\\"percentStraight\\\":\";\n"
      "  json += String(live.percent_straight, 1);\n"
      "  json += \",\\\"angleDeg\\\":\";\n"
      "  json += String(live.knee_angle_deg, 1);\n"
      "  json += \",\\\"speed\\\":\";\n"
      "  json += String(live.speed_percent_per_sec, 1);\n"
      "  json += \",\\\"accel\\\":\";\n"
      "  json += String(live.accel_percent_per_sec2, 1);\n"
      "  json += \",\\\"inStep\\\":\";\n"
      "  json += live.in_step ? \"true\" : \"false\";\n"
      "  json += \",\\\"lastScore\\\":\";\n"
      "  json += String(live.last_score, 1);\n"
      "  json += \",\\\"stepCount\\\":\";\n"
      "  json += String(live.step_count);\n"
      "  json += \",\\\"todayAverage\\\":\";\n"
      "  json += String(rehab_today_average_score(&storage, time_synced, current_epoch_ms, timezone_offset_minutes), 1);\n"
      "  json += \",\\\"goal\\\":\";\n"
      "  json += String(DAILY_GOAL_SCORE);\n"
      "  json += \",\\\"timeSynced\\\":\";\n"
      "  json += time_synced ? \"true\" : \"false\";\n"
      "  json += \"}\";\n"
      "  return json;\n"
      "}\n\n"
      "String generated_web_build_variables_json(const RehabConfig &rehab_config) {\n"
      "  String json;\n"
      "  json.reserve(480);\n"
      "  json += \"{\";\n"
      "  json += \"\\\"minReading\\\":\";\n"
      "  json += String(rehab_config.min_reading);\n"
      "  json += \",\\\"maxReading\\\":\";\n"
      "  json += String(rehab_config.max_reading);\n"
      "  json += \",\\\"bentAngle\\\":\";\n"
      "  json += String(rehab_config.bent_angle, 2);\n"
      "  json += \",\\\"straightAngle\\\":\";\n"
      "  json += String(rehab_config.straight_angle, 2);\n"
      "  json += \",\\\"sampleIntervalMs\\\":\";\n"
      "  json += String(rehab_config.sample_interval_ms);\n"
      "  json += \",\\\"filterAlpha\\\":\";\n"
      "  json += String(rehab_config.filter_alpha, 3);\n"
      "  json += \",\\\"motionThreshold\\\":\";\n"
      "  json += String(rehab_config.motion_threshold, 3);\n"
      "  json += \",\\\"stepRangeThreshold\\\":\";\n"
      "  json += String(rehab_config.step_range_threshold, 3);\n"
      "  json += \",\\\"startReadyThreshold\\\":\";\n"
      "  json += String(rehab_config.start_ready_threshold, 3);\n"
      "  json += \",\\\"returnMargin\\\":\";\n"
      "  json += String(rehab_config.return_margin, 3);\n"
      "  json += \",\\\"maxStepDurationMs\\\":\";\n"
      "  json += String(rehab_config.max_step_duration_ms);\n"
      "  json += \",\\\"sampleHistoryEnabled\\\":\";\n"
      "  json += ENABLE_SAMPLE_HISTORY ? \"true\" : \"false\";\n"
      "  json += \"}\";\n"
      "  return json;\n"
      "}\n\n"
      "String generated_web_build_history_json(const PersistedHistory &storage,\n"
      "                                        int timezone_offset_minutes) {\n"
      "  DaySummary summaries[14] = {};\n"
      "  size_t summary_count = 0;\n"
      "  String json;\n"
      "  for (int i = static_cast<int>(storage.count) - 1; i >= 0; --i) {\n"
      "    const int64_t key = rehab_local_day_key(storage.records[i].timestamp_ms, timezone_offset_minutes);\n"
      "    if (key < 0) {\n"
      "      continue;\n"
      "    }\n"
      "    bool found = false;\n"
      "    for (size_t s = 0; s < summary_count; ++s) {\n"
      "      if (summaries[s].key == key) {\n"
      "        summaries[s].total += storage.records[i].score;\n"
      "        ++summaries[s].count;\n"
      "        found = true;\n"
      "        break;\n"
      "      }\n"
      "    }\n"
      "    if (!found && summary_count < 14) {\n"
      "      summaries[summary_count].key = key;\n"
      "      summaries[summary_count].total = storage.records[i].score;\n"
      "      summaries[summary_count].count = 1;\n"
      "      ++summary_count;\n"
      "    }\n"
      "  }\n"
      "  json.reserve(22000);\n"
      "  json += \"{\";\n"
      "  json += \"\\\"goal\\\":\";\n"
      "  json += String(DAILY_GOAL_SCORE);\n"
      "  json += \",\\\"history\\\":[\";\n"
      "  for (int i = static_cast<int>(storage.count) - 1; i >= 0; --i) {\n"
      "    const StepRecord &record = storage.records[i];\n"
      "    if (i != static_cast<int>(storage.count) - 1) {\n"
      "      json += \",\";\n"
      "    }\n"
      "    json += \"{\";\n"
      "    json += \"\\\"id\\\":\";\n"
      "    json += String(record.id);\n"
      "    json += \",\\\"timestampMs\\\":\";\n"
      "    json += String(static_cast<unsigned long long>(record.timestamp_ms));\n"
      "    json += \",\\\"score\\\":\";\n"
      "    json += String(record.score, 1);\n"
      "    json += \",\\\"shakiness\\\":\";\n"
      "    json += String(record.shakiness_penalty, 1);\n"
      "    json += \",\\\"uncontrolledDescent\\\":\";\n"
      "    json += String(record.descent_penalty, 1);\n"
      "    json += \",\\\"compensation\\\":\";\n"
      "    json += String(record.compensation_penalty, 1);\n"
      "    json += \",\\\"durationMs\\\":\";\n"
      "    json += String(record.duration_ms, 0);\n"
      "    json += \",\\\"descentMs\\\":\";\n"
      "    json += String(record.descent_ms, 0);\n"
      "    json += \",\\\"ascentMs\\\":\";\n"
      "    json += String(record.ascent_ms, 0);\n"
      "    json += \",\\\"range\\\":\";\n"
      "    json += String(record.range, 1);\n"
      "    json += \",\\\"startPercent\\\":\";\n"
      "    json += String(record.start_percent, 1);\n"
      "    json += \",\\\"endPercent\\\":\";\n"
      "    json += String(record.end_percent, 1);\n"
      "    json += \",\\\"minPercent\\\":\";\n"
      "    json += String(record.min_percent, 1);\n"
      "    json += \",\\\"maxPercent\\\":\";\n"
      "    json += String(record.max_percent, 1);\n"
      "    json += \",\\\"descentAvgSpeed\\\":\";\n"
      "    json += String(record.descent_avg_speed, 1);\n"
      "    json += \",\\\"ascentAvgSpeed\\\":\";\n"
      "    json += String(record.ascent_avg_speed, 1);\n"
      "    json += \",\\\"descentPeakSpeed\\\":\";\n"
      "    json += String(record.descent_peak_speed, 1);\n"
      "    json += \",\\\"ascentPeakSpeed\\\":\";\n"
      "    json += String(record.ascent_peak_speed, 1);\n"
      "    json += \",\\\"oscillations\\\":\";\n"
      "    json += String(record.oscillation_count);\n"
      "    json += \"}\";\n"
      "  }\n"
      "  json += \"],\\\"dailyAverages\\\":[\";\n"
      "  for (size_t i = 0; i < summary_count; ++i) {\n"
      "    const int64_t day_start_utc_ms = summaries[i].key * 86400000LL + static_cast<int64_t>(timezone_offset_minutes) * 60000LL;\n"
      "    const float average = summaries[i].total / static_cast<float>(summaries[i].count > 0 ? summaries[i].count : 1);\n"
      "    if (i != 0) {\n"
      "      json += \",\";\n"
      "    }\n"
      "    json += \"{\";\n"
      "    json += \"\\\"dayStartMs\\\":\";\n"
      "    json += String(static_cast<long long>(day_start_utc_ms));\n"
      "    json += \",\\\"averageScore\\\":\";\n"
      "    json += String(average, 1);\n"
      "    json += \",\\\"count\\\":\";\n"
      "    json += String(summaries[i].count);\n"
      "    json += \"}\";\n"
      "  }\n"
      "  json += \"]}\";\n"
      "  return json;\n"
      "}\n";

  return generator_write_text_asset(output_dir, "web_runtime_generated.cpp", source_text);
}

static const char *generator_embedded_js(void) {
  return
      "(function () {\n"
      "  class RehabTrackerApp {\n"
      "    constructor() {\n"
      "      this.state = {\n"
      "        selectedView: window.location.pathname === '/variables' ? 'variables' : 'live',\n"
      "        goal: 85,\n"
      "        live: { reading: 0, percentStraight: 0, speed: 0, inStep: false, stepCount: 0, lastScore: 0, todayAverage: 0, timeSynced: false },\n"
      "        history: [],\n"
      "        dailyAverages: [],\n"
      "        latestStep: null,\n"
      "        variables: {}\n"
      "      };\n"
      "      this.refs = {\n"
      "        tabs: document.getElementById('tabs'),\n"
      "        liveView: document.getElementById('live-view'),\n"
      "        historyView: document.getElementById('history-view'),\n"
      "        variablesView: document.getElementById('variables-view'),\n"
      "        reading: document.getElementById('reading'),\n"
      "        percentStraight: document.getElementById('percent-straight'),\n"
      "        lastScore: document.getElementById('last-score'),\n"
      "        stepCount: document.getElementById('step-count'),\n"
      "        todayAverage: document.getElementById('today-average'),\n"
      "        goalInline: document.getElementById('goal-inline'),\n"
      "        speed: document.getElementById('speed'),\n"
      "        syncPill: document.getElementById('sync-pill'),\n"
      "        stepStatus: document.getElementById('step-status'),\n"
      "        liveShaky: document.getElementById('live-shaky'),\n"
      "        liveDescent: document.getElementById('live-descent'),\n"
      "        liveCompensation: document.getElementById('live-compensation'),\n"
      "        lowerLeg: document.getElementById('lower-leg'),\n"
      "        ankle: document.getElementById('ankle'),\n"
      "        historyBody: document.getElementById('history-body'),\n"
      "        dailyAverageBody: document.getElementById('daily-average-body'),\n"
      "        variablesStatus: document.getElementById('variables-status'),\n"
      "        saveVariables: document.getElementById('save-variables'),\n"
      "        reloadVariables: document.getElementById('reload-variables'),\n"
      "        variableInputs: Array.from(document.querySelectorAll('#variables-view input'))\n"
      "      };\n"
      "    }\n"
      "    async init() {\n"
      "      this.attachEvents();\n"
      "      this.renderView();\n"
      "      await this.syncClock();\n"
      "      await Promise.all([this.refreshLive(), this.refreshHistory(), this.refreshVariables()]);\n"
      "      window.setInterval(() => this.refreshLive(), 250);\n"
      "      window.setInterval(() => this.refreshHistory(), 3000);\n"
      "    }\n"
      "    attachEvents() {\n"
      "      if (this.refs.tabs) {\n"
      "        this.refs.tabs.addEventListener('click', (event) => {\n"
      "          const button = event.target.closest('button[data-view]');\n"
      "          if (!button) { return; }\n"
      "          this.switchView(button.dataset.view);\n"
      "        });\n"
      "      }\n"
      "      if (this.refs.saveVariables) {\n"
      "        this.refs.saveVariables.addEventListener('click', () => this.saveVariables());\n"
      "      }\n"
      "      if (this.refs.reloadVariables) {\n"
      "        this.refs.reloadVariables.addEventListener('click', () => this.refreshVariables());\n"
      "      }\n"
      "    }\n"
      "    switchView(view) {\n"
      "      this.state.selectedView = view;\n"
      "      this.renderView();\n"
      "      window.history.replaceState({}, '', view === 'variables' ? '/variables' : '/');\n"
      "    }\n"
      "    renderView() {\n"
      "      const view = this.state.selectedView;\n"
      "      if (this.refs.liveView) { this.refs.liveView.classList.toggle('hidden', view !== 'live'); }\n"
      "      if (this.refs.historyView) { this.refs.historyView.classList.toggle('hidden', view !== 'history'); }\n"
      "      if (this.refs.variablesView) { this.refs.variablesView.classList.toggle('hidden', view !== 'variables'); }\n"
      "      if (this.refs.tabs) {\n"
      "        Array.from(this.refs.tabs.querySelectorAll('button[data-view]')).forEach((button) => {\n"
      "          button.classList.toggle('active', button.dataset.view === view);\n"
      "        });\n"
      "      }\n"
      "    }\n"
      "    async syncClock() {\n"
      "      const epochMs = Date.now();\n"
      "      const offsetMinutes = new Date().getTimezoneOffset();\n"
      "      try {\n"
      "        await fetch('/api/time-sync?epochMs=' + epochMs + '&offsetMinutes=' + offsetMinutes);\n"
      "      } catch (error) {}\n"
      "    }\n"
      "    clamp(value, min, max) {\n"
      "      return Math.min(max, Math.max(min, value));\n"
      "    }\n"
      "    mapPercentToAngle(percentStraight) {\n"
      "      const normalized = this.clamp(percentStraight / 100, 0, 1);\n"
      "      return 78 + (0 - 78) * normalized;\n"
      "    }\n"
      "    updateKnee(percentStraight) {\n"
      "      const angle = this.mapPercentToAngle(percentStraight);\n"
      "      const radians = angle * Math.PI / 180;\n"
      "      const kneeX = 140;\n"
      "      const kneeY = 174;\n"
      "      const shinLength = 100;\n"
      "      const ankleX = kneeX + Math.sin(radians) * shinLength;\n"
      "      const ankleY = kneeY + Math.cos(radians) * shinLength;\n"
      "      if (this.refs.lowerLeg) {\n"
      "        this.refs.lowerLeg.setAttribute('x2', ankleX.toFixed(1));\n"
      "        this.refs.lowerLeg.setAttribute('y2', ankleY.toFixed(1));\n"
      "      }\n"
      "      if (this.refs.ankle) {\n"
      "        this.refs.ankle.setAttribute('cx', ankleX.toFixed(1));\n"
      "        this.refs.ankle.setAttribute('cy', ankleY.toFixed(1));\n"
      "      }\n"
      "    }\n"
      "    formatTimestamp(timestampMs) {\n"
      "      if (!timestampMs) { return 'Unsynced'; }\n"
      "      return new Date(timestampMs).toLocaleString();\n"
      "    }\n"
      "    scoreChipClass(score) {\n"
      "      if (score < 55) { return 'score-chip bad'; }\n"
      "      if (score < 75) { return 'score-chip warn'; }\n"
      "      return 'score-chip';\n"
      "    }\n"
      "    async refreshLive() {\n"
      "      try {\n"
      "        const response = await fetch('/api/live');\n"
      "        const payload = await response.json();\n"
      "        this.state.live = payload;\n"
      "        this.state.goal = payload.goal;\n"
      "        this.renderLive();\n"
      "      } catch (error) {\n"
      "        if (this.refs.syncPill) {\n"
      "          this.refs.syncPill.textContent = 'Live signal unavailable';\n"
      "          this.refs.syncPill.className = 'status-pill warn';\n"
      "        }\n"
      "      }\n"
      "    }\n"
      "    async refreshHistory() {\n"
      "      try {\n"
      "        const response = await fetch('/api/history');\n"
      "        const payload = await response.json();\n"
      "        this.state.history = payload.history || [];\n"
      "        this.state.dailyAverages = payload.dailyAverages || [];\n"
      "        this.state.goal = payload.goal || this.state.goal;\n"
      "        this.state.latestStep = this.state.history.length > 0 ? this.state.history[0] : null;\n"
      "        this.renderHistory();\n"
      "        this.renderLiveQuality();\n"
      "      } catch (error) {}\n"
      "    }\n"
      "    async refreshVariables() {\n"
      "      try {\n"
      "        const response = await fetch('/api/variables');\n"
      "        const payload = await response.json();\n"
      "        this.state.variables = payload;\n"
      "        this.renderVariables();\n"
      "        if (this.refs.variablesStatus) {\n"
      "          this.refs.variablesStatus.textContent = 'Variables loaded';\n"
      "          this.refs.variablesStatus.className = 'status-pill';\n"
      "        }\n"
      "      } catch (error) {\n"
      "        if (this.refs.variablesStatus) {\n"
      "          this.refs.variablesStatus.textContent = 'Could not load variables';\n"
      "          this.refs.variablesStatus.className = 'status-pill warn';\n"
      "        }\n"
      "      }\n"
      "    }\n"
      "    async saveVariables() {\n"
      "      const form = new URLSearchParams();\n"
      "      this.refs.variableInputs.forEach((input) => {\n"
      "        if (!input.disabled) { form.append(input.name, input.value); }\n"
      "      });\n"
      "      try {\n"
      "        const response = await fetch('/api/variables', {\n"
      "          method: 'POST',\n"
      "          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },\n"
      "          body: form.toString()\n"
      "        });\n"
      "        if (!response.ok) { throw new Error('save failed'); }\n"
      "        const payload = await response.json();\n"
      "        this.state.variables = payload;\n"
      "        this.renderVariables();\n"
      "        if (this.refs.variablesStatus) {\n"
      "          this.refs.variablesStatus.textContent = 'Variables applied live';\n"
      "          this.refs.variablesStatus.className = 'status-pill';\n"
      "        }\n"
      "        await this.refreshLive();\n"
      "      } catch (error) {\n"
      "        if (this.refs.variablesStatus) {\n"
      "          this.refs.variablesStatus.textContent = 'Save failed. Check variable values.';\n"
      "          this.refs.variablesStatus.className = 'status-pill warn';\n"
      "        }\n"
      "      }\n"
      "    }\n"
      "    renderVariables() {\n"
      "      Object.entries(this.state.variables).forEach(([key, value]) => {\n"
      "        const input = document.getElementById(key);\n"
      "        if (input) { input.value = value; }\n"
      "      });\n"
      "    }\n"
      "    renderLive() {\n"
      "      const live = this.state.live;\n"
      "      if (this.refs.reading) { this.refs.reading.textContent = String(live.reading); }\n"
      "      if (this.refs.percentStraight) { this.refs.percentStraight.textContent = live.percentStraight.toFixed(1) + '%'; }\n"
      "      if (this.refs.lastScore) { this.refs.lastScore.textContent = live.lastScore.toFixed(1); }\n"
      "      if (this.refs.stepCount) { this.refs.stepCount.textContent = String(live.stepCount); }\n"
      "      if (this.refs.todayAverage) { this.refs.todayAverage.textContent = live.todayAverage.toFixed(1); }\n"
      "      if (this.refs.goalInline) { this.refs.goalInline.textContent = String(this.state.goal); }\n"
      "      if (this.refs.speed) { this.refs.speed.textContent = live.speed.toFixed(1); }\n"
      "      if (this.refs.stepStatus) {\n"
      "        this.refs.stepStatus.textContent = live.inStep ? 'Capturing active movement cycle' : 'Waiting for a full movement cycle';\n"
      "        this.refs.stepStatus.className = live.inStep ? 'status-pill' : 'status-pill warn';\n"
      "      }\n"
      "      if (this.refs.syncPill) {\n"
      "        this.refs.syncPill.textContent = live.timeSynced ? 'Phone time synced for daily tracking' : 'Waiting for phone time sync';\n"
      "        this.refs.syncPill.className = live.timeSynced ? 'status-pill' : 'status-pill warn';\n"
      "      }\n"
      "      this.updateKnee(live.percentStraight);\n"
      "      this.renderLiveQuality();\n"
      "    }\n"
      "    renderLiveQuality() {\n"
      "      const latest = this.state.latestStep;\n"
      "      if (!latest) {\n"
      "        if (this.refs.liveShaky) { this.refs.liveShaky.textContent = '0.0'; }\n"
      "        if (this.refs.liveDescent) { this.refs.liveDescent.textContent = '0.0'; }\n"
      "        if (this.refs.liveCompensation) { this.refs.liveCompensation.textContent = '0.0'; }\n"
      "        return;\n"
      "      }\n"
      "      if (this.refs.liveShaky) { this.refs.liveShaky.textContent = latest.shakiness.toFixed(1); }\n"
      "      if (this.refs.liveDescent) { this.refs.liveDescent.textContent = latest.uncontrolledDescent.toFixed(1); }\n"
      "      if (this.refs.liveCompensation) { this.refs.liveCompensation.textContent = latest.compensation.toFixed(1); }\n"
      "    }\n"
      "    renderHistory() {\n"
      "      const rows = this.state.history;\n"
      "      const daily = this.state.dailyAverages;\n"
      "      if (this.refs.historyBody) {\n"
      "        if (!rows.length) {\n"
      "          this.refs.historyBody.innerHTML = '<tr><td class=\"empty\" colspan=\"10\">No steps recorded yet.</td></tr>';\n"
      "        } else {\n"
      "          this.refs.historyBody.innerHTML = rows.map((step) => `\n"
      "            <tr>\n"
      "              <td>${this.formatTimestamp(step.timestampMs)}</td>\n"
      "              <td><span class=\"${this.scoreChipClass(step.score)}\">${step.score.toFixed(1)}</span></td>\n"
      "              <td>${step.shakiness.toFixed(1)}</td>\n"
      "              <td>${step.uncontrolledDescent.toFixed(1)}</td>\n"
      "              <td>${step.compensation.toFixed(1)}</td>\n"
      "              <td>${Math.round(step.durationMs)} ms</td>\n"
      "              <td>${step.range.toFixed(1)}%</td>\n"
      "              <td>${step.descentAvgSpeed.toFixed(1)}</td>\n"
      "              <td>${step.ascentAvgSpeed.toFixed(1)}</td>\n"
      "              <td>${step.oscillations}</td>\n"
      "            </tr>`).join('');\n"
      "        }\n"
      "      }\n"
      "      if (this.refs.dailyAverageBody) {\n"
      "        if (!daily.length) {\n"
      "          this.refs.dailyAverageBody.innerHTML = '<tr><td class=\"empty\" colspan=\"4\">No synced history yet.</td></tr>';\n"
      "        } else {\n"
      "          this.refs.dailyAverageBody.innerHTML = daily.map((day) => `\n"
      "            <tr>\n"
      "              <td>${this.formatTimestamp(day.dayStartMs).split(',')[0]}</td>\n"
      "              <td><span class=\"${this.scoreChipClass(day.averageScore)}\">${day.averageScore.toFixed(1)}</span></td>\n"
      "              <td>${day.count}</td>\n"
      "              <td>${this.state.goal}</td>\n"
      "            </tr>`).join('');\n"
      "        }\n"
      "      }\n"
      "    }\n"
      "  }\n"
      "  const app = new RehabTrackerApp();\n"
      "  app.init();\n"
      "}());\n";
}

int generator_prepare_output_directory(const char *output_dir) {
  LOG_TRACE("generator_prepare_output_directory output_dir=%s\n", output_dir);
  return output_fs_prepare_clean_directory(output_dir);
}

int generator_generate_embedded_bundle(const char *output_dir,
                                       const char *html_source,
                                       const char *css_source) {
  const char *js_source = generator_embedded_js();

  LOG_TRACE("generator_generate_embedded_bundle output_dir=%s html_bytes=%zu css_bytes=%zu js_bytes=%zu\n",
            output_dir,
            html_source != NULL ? strlen(html_source) : 0u,
            css_source != NULL ? strlen(css_source) : 0u,
            strlen(js_source));

  if (generator_write_text_asset(output_dir, "index.html", html_source) != 0) {
    return 1;
  }
  if (generator_write_text_asset(output_dir, "styles.css", css_source) != 0) {
    return 1;
  }
  if (generator_write_text_asset(output_dir, "app.js", js_source) != 0) {
    return 1;
  }
  if (generator_emit_runtime_header(output_dir) != 0) {
    return 1;
  }
  if (generator_emit_runtime_source(output_dir) != 0) {
    return 1;
  }
  if (generator_emit_cpp_bundle(output_dir, html_source, css_source, js_source) != 0) {
    return 1;
  }

  return 0;
}

int generator_validate_embedded_bundle(const char *output_dir) {
  size_t index;

  for (index = 0; index < sizeof(g_generated_files) / sizeof(g_generated_files[0]); ++index) {
    char path[512];

    generator_build_path(path, sizeof(path), output_dir, g_generated_files[index]);
    if (!output_fs_file_exists(path)) {
      log_errorf("generated embedded file missing: %s\n", path);
      return 1;
    }
  }

  log_printf("EMBEDDED GENERATION VALIDATION OK\n");
  return 0;
}
