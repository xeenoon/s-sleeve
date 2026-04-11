#include <Arduino.h>
#include <WebServer.h>

#include <string.h>

#include "generated/web_runtime_generated.h"
#include "web_page.h"

static const char g_route_body_0[] PROGMEM = "{\"goal\":85,\"history\":[{\"timestampMs\":1713013200000,\"score\":92.0,\"shakiness\":1.2,\"uncontrolledDescent\":0.4,\"compensation\":0.3,\"durationMs\":1280.0,\"range\":67,\"descentAvgSpeed\":0.8,\"ascentAvgSpeed\":0.9,\"oscillations\":1},{\"timestampMs\":1713016800000,\"score\":88.0,\"shakiness\":1.0,\"uncontrolledDescent\":0.5,\"compensation\":0.2,\"durationMs\":1335.0,\"range\":67,\"descentAvgSpeed\":0.7,\"ascentAvgSpeed\":0.8,\"oscillations\":0}],\"dailyAverages\":[{\"dayStartMs\":1712966400000,\"averageScore\":92.0,\"count\":1},{\"dayStartMs\":1713052800000,\"averageScore\":90.0,\"count\":2}]}\n";
static const char g_route_body_1[] PROGMEM = "{\"reading\":3500,\"percentStraight\":67,\"angleDeg\":25.7,\"speed\":0.0,\"inStep\":false,\"stepCount\":1,\"lastScore\":92.0,\"todayAverage\":92.0,\"timeSynced\":true,\"goal\":85,\"shaky\":1.2,\"uncontrolledDescent\":0.4,\"compensation\":0.3}\n";
static const char g_route_body_2[] PROGMEM = "{\"ok\":true}\n";
static const char g_route_body_3[] PROGMEM = "{\"minReading\":3100,\"maxReading\":3700,\"bentAngle\":78.0,\"straightAngle\":0.0,\"sampleIntervalMs\":50,\"filterAlpha\":0.180,\"motionThreshold\":0.020,\"stepRangeThreshold\":18.0,\"startReadyThreshold\":8.0,\"returnMargin\":5.0,\"maxStepDurationMs\":2600,\"sampleHistoryEnabled\":\"true\",\"status\":\"Variables loaded\",\"goal\":85}\n";
static const char g_route_body_4[] PROGMEM = "{\"reading\":3500,\"percentStraight\":67,\"hasSignal\":true,\"goal\":85,\"todayAverage\":92.0,\"speed\":0.0,\"lastScore\":92.0,\"stepCount\":1,\"timeSynced\":true,\"inStep\":false,\"shaky\":1.2,\"uncontrolledDescent\":0.4,\"compensation\":0.3,\"lowerLegX2\":\"183.4\",\"lowerLegY2\":\"264.1\",\"ankleX\":\"183.4\",\"ankleY\":\"264.1\"}\n";
static const char g_route_body_5[] PROGMEM = "3500\n";
static const char g_route_body_6[] PROGMEM = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"utf-8\">\n  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n  <title>Knee Rehab Tracker</title>\n  <link rel=\"stylesheet\" href=\"/styles.css\">\n</head>\n<body>\n  <div class=\"shell\">\n    <section class=\"hero\">\n      <div class=\"panel\">\n        <p class=\"eyebrow\">Rehab Tracker</p>\n        <h1 class=\"headline\"><span id=\"today-average\">0.0</span>/100</h1>\n        <p class=\"subline\">Daily average step quality with a goal of <strong id=\"goal-inline\">85</strong>.</p>\n      </div>\n      <div class=\"panel\">\n        <div class=\"status-row\">\n          <div class=\"tabs\" id=\"tabs\">\n            <button data-view=\"live\" class=\"active\" type=\"button\">Live</button>\n            <button data-view=\"history\" type=\"button\">History</button>\n            <button data-view=\"variables\" type=\"button\">Variables</button>\n          </div>\n          <div id=\"sync-pill\" class=\"status-pill\">Phone time synced for daily tracking</div>\n        </div>\n      </div>\n    </section>\n    <section id=\"live-view\" class=\"hidden\"></section>\n    <section id=\"history-view\" class=\"hidden\"></section>\n    <section id=\"variables-view\">\n      <div class=\"panel\">\n        <p class=\"eyebrow\">Calibration Variables</p>\n        <div id=\"variables-status\" class=\"status-pill warn\">Variables not loaded yet</div>\n      </div>\n    </section>\n  </div>\n  <script src=\"/app.js\"></script>\n</body>\n</html>\n";
static const char g_route_body_7[] PROGMEM = "{\"ok\":true,\"status\":\"Variables saved\"}\n";

static const generated_web_static_route_t g_generated_routes[] = {
  { "GET", "/api/history", "application/json", g_route_body_0 },
  { "GET", "/api/live", "application/json", g_route_body_1 },
  { "GET", "/api/time-sync", "application/json", g_route_body_2 },
  { "GET", "/api/variables", "application/json", g_route_body_3 },
  { "GET", "/state", "application/json", g_route_body_4 },
  { "GET", "/value", "text/plain; charset=utf-8", g_route_body_5 },
  { "GET", "/variables", "text/html; charset=utf-8", g_route_body_6 },
  { "POST", "/api/variables", "application/json", g_route_body_7 },
};

static const char *generated_web_method_name(HTTPMethod method) {
  switch (method) {
    case HTTP_GET: return "GET";
    case HTTP_POST: return "POST";
    case HTTP_PUT: return "PUT";
    case HTTP_DELETE: return "DELETE";
    case HTTP_PATCH: return "PATCH";
    case HTTP_OPTIONS: return "OPTIONS";
    default: return "GET";
  }
}

void generated_web_send_root(WebServer &server) {
  server.send_P(200, "text/html", INDEX_HTML);
}

void generated_web_send_styles(WebServer &server) {
  server.send_P(200, "text/css", STYLES_CSS);
}

void generated_web_send_app_js(WebServer &server) {
  server.send_P(200, "application/javascript", APP_JS);
}

size_t generated_web_static_route_count(void) {
  return sizeof(g_generated_routes) / sizeof(g_generated_routes[0]);
}

const generated_web_static_route_t *generated_web_static_route_at(size_t index) {
  if (index >= generated_web_static_route_count()) {
    return nullptr;
  }
  return &g_generated_routes[index];
}

bool generated_web_try_send_static_route(WebServer &server, const String &path, HTTPMethod method) {
  const char *method_name = generated_web_method_name(method);
  size_t index;
  for (index = 0; index < generated_web_static_route_count(); ++index) {
    const generated_web_static_route_t &route = g_generated_routes[index];
    if (strcmp(route.method, method_name) == 0 && path.equals(route.path)) {
      server.send_P(200, route.content_type, route.body);
      return true;
    }
  }
  return false;
}
