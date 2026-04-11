#include <Arduino.h>
#include <WebServer.h>

#include <string.h>

#include "generated/web_runtime_generated.h"
#include "web_page.h"

static const char g_route_body_0[] PROGMEM = "{\r\n    \"goal\":  85,\r\n    \"history\":  [\r\n                    {\r\n                        \"timestampMs\":  1775030400000,\r\n                        \"score\":  56.8,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  980,\r\n                        \"range\":  52,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775036100000,\r\n                        \"score\":  64.4,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1023,\r\n                        \"range\":  56,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775041800000,\r\n                        \"score\":  72,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1066,\r\n                        \"range\":  60,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775047500000,\r\n                        \"score\":  79.6,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1109,\r\n                        \"range\":  64,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775053200000,\r\n                        \"score\":  87.2,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1152,\r\n                        \"range\":  68,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775058900000,\r\n                        \"score\":  91.8,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  1,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1195,\r\n                        \"range\":  72,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  0.8,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775064600000,\r\n                        \"score\":  60.4,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.5,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1238,\r\n                        \"range\":  76,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1.3,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775070300000,\r\n                        \"score\":  68,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  1.1,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1281,\r\n                        \"range\":  80,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  0.7,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775076000000,\r\n                        \"score\":  75.6,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.6,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1324,\r\n                        \"range\":  53,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  1.2,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775081700000,\r\n                        \"score\":  83.2,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.2,\r\n                        \"compensation\":  0.8,\r\n                        \"durationMs\":  1367,\r\n                        \"range\":  57,\r\n                        \"descentAvgSpeed\":  1.5,\r\n                        \"ascentAvgSpeed\":  0.6,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775087400000,\r\n                        \"score\":  87.8,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  0.8,\r\n                        \"compensation\":  0.1,\r\n                        \"durationMs\":  1410,\r\n                        \"range\":  61,\r\n                        \"descentAvgSpeed\":  0.5,\r\n                        \"ascentAvgSpeed\":  1,\r\n                        \"oscillations\":  2\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775093100000,\r\n                        \"score\":  95.4,\r\n                        \"shakiness\":  2.3,\r\n                        \"uncontrolledDescent\":  0.3,\r\n                        \"compensation\":  0.3,\r\n                        \"durationMs\":  1453,\r\n                        \"range\":  65,\r\n                        \"descentAvgSpeed\":  0.7,\r\n                        \"ascentAvgSpeed\":  1.5,\r\n                        \"oscillations\":  1\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775098800000,\r\n                        \"score\":  64,\r\n                        \"shakiness\":  0.7,\r\n                        \"uncontrolledDescent\":  0.9,\r\n                        \"compensation\":  0.5,\r\n                        \"durationMs\":  1496,\r\n                        \"range\":  69,\r\n                        \"descentAvgSpeed\":  1,\r\n                        \"ascentAvgSpeed\":  0.9,\r\n                        \"oscillations\":  0\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775104500000,\r\n                        \"score\":  71.6,\r\n                        \"shakiness\":  1.2,\r\n                        \"uncontrolledDescent\":  0.4,\r\n                        \"compensation\":  0.6,\r\n                        \"durationMs\":  1539,\r\n                        \"range\":  73,\r\n                        \"descentAvgSpeed\":  1.2,\r\n                        \"ascentAvgSpeed\":  1.4,\r\n                        \"oscillations\":  3\r\n                    },\r\n                    {\r\n                        \"timestampMs\":  1775110200000,\r\n                        \"score\":  79.2,\r\n                        \"shakiness\":  1.8,\r\n                        \"uncontrolledDescent\":  1,\r\n                        \"compensation\":  0.8,\r\n                    ";
static const char g_route_body_1[] PROGMEM = "{\"reading\":3500,\"percentStraight\":67,\"angleDeg\":25.7,\"speed\":0.0,\"inStep\":false,\"stepCount\":1,\"lastScore\":92.0,\"todayAverage\":92.0,\"timeSynced\":true,\"goal\":85,\"shaky\":1.2,\"uncontrolledDescent\":0.4,\"compensation\":0.3}\n";
static const char g_route_body_2[] PROGMEM = "{\"ok\":true}\n";
static const char g_route_body_3[] PROGMEM = "{\"minReading\":3100,\"maxReading\":3700,\"bentAngle\":78.0,\"straightAngle\":0.0,\"sampleIntervalMs\":50,\"filterAlpha\":0.180,\"motionThreshold\":0.020,\"stepRangeThreshold\":18.0,\"startReadyThreshold\":8.0,\"returnMargin\":5.0,\"maxStepDurationMs\":2600,\"sampleHistoryEnabled\":\"true\",\"status\":\"Variables loaded\",\"goal\":85}\n";
static const char g_route_body_4[] PROGMEM = "{\"reading\":3500,\"percentStraight\":67,\"hasSignal\":true,\"goal\":85,\"todayAverage\":92.0,\"speed\":0.0,\"lastScore\":92.0,\"stepCount\":1,\"timeSynced\":true,\"inStep\":false,\"shaky\":1.2,\"uncontrolledDescent\":0.4,\"compensation\":0.3,\"lowerLegX2\":\"183.4\",\"lowerLegY2\":\"264.1\",\"ankleX\":\"183.4\",\"ankleY\":\"264.1\"}\n";
static const char g_route_body_5[] PROGMEM = "3500\n";
static const char g_route_body_6[] PROGMEM = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"utf-8\">\n  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n  <title>s-sleeve</title>\n  <link rel=\"icon\" type=\"image/svg+xml\" href=\"data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'%3E%3Cdefs%3E%3CradialGradient id='g' cx='30%25' cy='28%25' r='72%25'%3E%3Cstop offset='0%25' stop-color='%23ffd6a8'/%3E%3Cstop offset='38%25' stop-color='%23ff9b43'/%3E%3Cstop offset='72%25' stop-color='%23d96a1d'/%3E%3Cstop offset='100%25' stop-color='%23993f0d'/%3E%3C/radialGradient%3E%3ClinearGradient id='s' x1='0' y1='0' x2='1' y2='1'%3E%3Cstop offset='0%25' stop-color='rgba(255,255,255,0.9)'/%3E%3Cstop offset='100%25' stop-color='rgba(255,255,255,0)'/%3E%3C/linearGradient%3E%3C/defs%3E%3Crect width='64' height='64' rx='18' fill='%23f6efe8'/%3E%3Ccircle cx='32' cy='32' r='21' fill='url(%23g)'/%3E%3Cellipse cx='25' cy='22' rx='11' ry='7' fill='url(%23s)' opacity='0.9'/%3E%3Ccircle cx='32' cy='32' r='21' fill='none' stroke='rgba(255,255,255,0.45)' stroke-width='1.5'/%3E%3C/svg%3E\">\n  <link rel=\"stylesheet\" href=\"/styles.css\">\n</head>\n<body>\n  <div class=\"shell\">\n    <section class=\"hero\">\n      <div class=\"panel\">\n        <p class=\"eyebrow\">s-sleeve</p>\n        <h1 class=\"headline\"><span id=\"today-average\">0.0</span>/100</h1>\n        <p class=\"subline\">s-sleeve live mobility tracking with a goal of <strong id=\"goal-inline\">85</strong>.</p>\n      </div>\n      <div class=\"panel\">\n        <div class=\"status-row\">\n          <div class=\"tabs\" id=\"tabs\">\n            <button data-view=\"live\" class=\"active\" type=\"button\">Live</button>\n            <button data-view=\"history\" type=\"button\">History</button>\n            <button data-view=\"variables\" type=\"button\">Variables</button>\n          </div>\n          <div id=\"sync-pill\" class=\"status-pill\">Phone time synced for daily tracking</div>\n        </div>\n      </div>\n    </section>\n    <section id=\"live-view\" class=\"hidden\"></section>\n    <section id=\"history-view\" class=\"hidden\"></section>\n    <section id=\"variables-view\">\n      <div class=\"panel\">\n        <p class=\"eyebrow\">Calibration Variables</p>\n        <div id=\"variables-status\" class=\"status-pill warn\">Variables not loaded yet</div>\n      </div>\n    </section>\n  </div>\n  <script src=\"/app.js\"></script>\n</body>\n</html>\n";
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
