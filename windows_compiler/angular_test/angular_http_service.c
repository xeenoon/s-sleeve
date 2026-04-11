#include "angular_http_service.h"

#include <stdio.h>
#include <string.h>

static const char g_route_body_0[] = "{\"goal\":85,\"history\":[{\"timestampMs\":1713013200000,\"score\":92.0,\"shakiness\":1.2,\"uncontrolledDescent\":0.4,\"compensation\":0.3,\"durationMs\":1280.0,\"range\":67,\"descentAvgSpeed\":0.8,\"ascentAvgSpeed\":0.9,\"oscillations\":1},{\"timestampMs\":1713016800000,\"score\":88.0,\"shakiness\":1.0,\"uncontrolledDescent\":0.5,\"compensation\":0.2,\"durationMs\":1335.0,\"range\":67,\"descentAvgSpeed\":0.7,\"ascentAvgSpeed\":0.8,\"oscillations\":0}],\"dailyAverages\":[{\"dayStartMs\":1712966400000,\"averageScore\":92.0,\"count\":1},{\"dayStartMs\":1713052800000,\"averageScore\":90.0,\"count\":2}]}\n";
static const char g_route_body_1[] = "{\"reading\":3500,\"percentStraight\":67,\"angleDeg\":25.7,\"speed\":0.0,\"inStep\":false,\"stepCount\":1,\"lastScore\":92.0,\"todayAverage\":92.0,\"timeSynced\":true,\"goal\":85,\"shaky\":1.2,\"uncontrolledDescent\":0.4,\"compensation\":0.3}\n";
static const char g_route_body_2[] = "{\"ok\":true}\n";
static const char g_route_body_3[] = "{\"minReading\":3100,\"maxReading\":3700,\"bentAngle\":78.0,\"straightAngle\":0.0,\"sampleIntervalMs\":50,\"filterAlpha\":0.180,\"motionThreshold\":0.020,\"stepRangeThreshold\":18.0,\"startReadyThreshold\":8.0,\"returnMargin\":5.0,\"maxStepDurationMs\":2600,\"sampleHistoryEnabled\":\"true\",\"status\":\"Variables loaded\",\"goal\":85}\n";
static const char g_route_body_4[] = "{\"reading\":3500,\"percentStraight\":67,\"hasSignal\":true,\"goal\":85,\"todayAverage\":92.0,\"speed\":0.0,\"lastScore\":92.0,\"stepCount\":1,\"timeSynced\":true,\"inStep\":false,\"shaky\":1.2,\"uncontrolledDescent\":0.4,\"compensation\":0.3,\"lowerLegX2\":\"183.4\",\"lowerLegY2\":\"264.1\",\"ankleX\":\"183.4\",\"ankleY\":\"264.1\"}\n";
static const char g_route_body_5[] = "3500\n";
static const char g_route_body_6[] = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"utf-8\">\n  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n  <title>Knee Rehab Tracker</title>\n  <link rel=\"stylesheet\" href=\"/styles.css\">\n</head>\n<body>\n  <div class=\"shell\">\n    <section class=\"hero\">\n      <div class=\"panel\">\n        <p class=\"eyebrow\">Rehab Tracker</p>\n        <h1 class=\"headline\"><span id=\"today-average\">0.0</span>/100</h1>\n        <p class=\"subline\">Daily average step quality with a goal of <strong id=\"goal-inline\">85</strong>.</p>\n      </div>\n      <div class=\"panel\">\n        <div class=\"status-row\">\n          <div class=\"tabs\" id=\"tabs\">\n            <button data-view=\"live\" class=\"active\" type=\"button\">Live</button>\n            <button data-view=\"history\" type=\"button\">History</button>\n            <button data-view=\"variables\" type=\"button\">Variables</button>\n          </div>\n          <div id=\"sync-pill\" class=\"status-pill\">Phone time synced for daily tracking</div>\n        </div>\n      </div>\n    </section>\n    <section id=\"live-view\" class=\"hidden\"></section>\n    <section id=\"history-view\" class=\"hidden\"></section>\n    <section id=\"variables-view\">\n      <div class=\"panel\">\n        <p class=\"eyebrow\">Calibration Variables</p>\n        <div id=\"variables-status\" class=\"status-pill warn\">Variables not loaded yet</div>\n      </div>\n    </section>\n  </div>\n  <script src=\"/app.js\"></script>\n</body>\n</html>\n";
static const char g_route_body_7[] = "{\"ok\":true,\"status\":\"Variables saved\"}\n";

static const angular_generated_route_t g_generated_routes[ANGULAR_GENERATED_ROUTE_COUNT > 0 ? ANGULAR_GENERATED_ROUTE_COUNT : 1] = {
  { "GET", "/api/history", "application/json", g_route_body_0 },
  { "GET", "/api/live", "application/json", g_route_body_1 },
  { "GET", "/api/time-sync", "application/json", g_route_body_2 },
  { "GET", "/api/variables", "application/json", g_route_body_3 },
  { "GET", "/state", "application/json", g_route_body_4 },
  { "GET", "/value", "text/plain; charset=utf-8", g_route_body_5 },
  { "GET", "/variables", "text/html; charset=utf-8", g_route_body_6 },
  { "POST", "/api/variables", "application/json", g_route_body_7 },
};

static void angular_http_log(const char *message) {
  fprintf(stdout, "[angular_http] %s\n", message);
  fflush(stdout);
}

static int angular_http_write_generated_route(void *context,
                                             const ng_http_request_t *request,
                                             ng_http_response_t *response) {
  const angular_generated_route_t *route = (const angular_generated_route_t *)context;
  (void)request;
  if (route == NULL) {
    response->status_code = 500;
    strcpy(response->body, "missing generated route context");
    return 0;
  }
  angular_http_log(route->path);
  snprintf(response->content_type, sizeof(response->content_type), "%s", route->content_type);
  snprintf(response->body, sizeof(response->body), "%s", route->body);
  return 0;
}

void angular_http_service_init(angular_http_service_t *service,
                               ng_runtime_t *runtime,
                               const char *html_page,
                               const char *css_text,
                               const char *js_text) {
  size_t index;
  service->runtime = runtime;
  for (index = 0; index < ANGULAR_GENERATED_ROUTE_COUNT; ++index) {
    service->generated_routes[index] = g_generated_routes[index];
  }
  service->routes[0].method = service->generated_routes[0].method;
  service->routes[0].path = service->generated_routes[0].path;
  service->routes[0].handler = angular_http_write_generated_route;
  service->routes[0].context = &service->generated_routes[0];
  service->routes[1].method = service->generated_routes[1].method;
  service->routes[1].path = service->generated_routes[1].path;
  service->routes[1].handler = angular_http_write_generated_route;
  service->routes[1].context = &service->generated_routes[1];
  service->routes[2].method = service->generated_routes[2].method;
  service->routes[2].path = service->generated_routes[2].path;
  service->routes[2].handler = angular_http_write_generated_route;
  service->routes[2].context = &service->generated_routes[2];
  service->routes[3].method = service->generated_routes[3].method;
  service->routes[3].path = service->generated_routes[3].path;
  service->routes[3].handler = angular_http_write_generated_route;
  service->routes[3].context = &service->generated_routes[3];
  service->routes[4].method = service->generated_routes[4].method;
  service->routes[4].path = service->generated_routes[4].path;
  service->routes[4].handler = angular_http_write_generated_route;
  service->routes[4].context = &service->generated_routes[4];
  service->routes[5].method = service->generated_routes[5].method;
  service->routes[5].path = service->generated_routes[5].path;
  service->routes[5].handler = angular_http_write_generated_route;
  service->routes[5].context = &service->generated_routes[5];
  service->routes[6].method = service->generated_routes[6].method;
  service->routes[6].path = service->generated_routes[6].path;
  service->routes[6].handler = angular_http_write_generated_route;
  service->routes[6].context = &service->generated_routes[6];
  service->routes[7].method = service->generated_routes[7].method;
  service->routes[7].path = service->generated_routes[7].path;
  service->routes[7].handler = angular_http_write_generated_route;
  service->routes[7].context = &service->generated_routes[7];
  ng_http_service_init(&service->service, html_page, css_text, js_text, service->routes, ANGULAR_GENERATED_ROUTE_COUNT);
}
