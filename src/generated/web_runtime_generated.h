#ifndef GENERATED_WEB_RUNTIME_H
#define GENERATED_WEB_RUNTIME_H

#include <Arduino.h>
#include <WebServer.h>

#include "rehab_metrics.h"
#include "rehab_types.h"

void generated_web_send_root(WebServer &server);
void generated_web_send_styles(WebServer &server);
void generated_web_send_app_js(WebServer &server);
void generated_web_send_variables_page(WebServer &server);
String generated_web_build_live_json(const LiveMetrics &live,
                                     const PersistedHistory &storage,
                                     bool time_synced,
                                     uint64_t current_epoch_ms,
                                     int timezone_offset_minutes);
String generated_web_build_variables_json(const RehabConfig &rehab_config);
String generated_web_build_history_json(const PersistedHistory &storage,
                                        int timezone_offset_minutes);

#endif
