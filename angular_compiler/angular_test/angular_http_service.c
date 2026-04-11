#include "angular_http_service.h"

#include <stdio.h>
#include <string.h>

#include "angular_mapPercentToAngle.h"
#include "angular_updateKnee.h"
#include "helpers/include/data/json_utils.h"
#include "helpers/include/math/number_utils.h"

static void angular_http_write_state_json(ng_runtime_t *runtime, char *buffer, size_t buffer_size) {
  snprintf(buffer,
           buffer_size,
           "{\"reading\":%d,\"percentStraight\":%d,\"hasSignal\":%s,\"lowerLegX2\":\"%s\",\"lowerLegY2\":\"%s\",\"ankleX\":\"%s\",\"ankleY\":\"%s\"}",
           ng_runtime_get_int(runtime, "reading", 0),
           ng_runtime_get_int(runtime, "percentStraight", 0),
           ng_runtime_get_bool(runtime, "hasSignal", 0) ? "true" : "false",
           ng_runtime_get_string(runtime, "lowerLegX2", "140.0"),
           ng_runtime_get_string(runtime, "lowerLegY2", "274.0"),
           ng_runtime_get_string(runtime, "ankleX", "140.0"),
           ng_runtime_get_string(runtime, "ankleY", "274.0"));
}

static int angular_http_percent_from_reading(int reading) {
  double normalized = ng_clamp_double(((double)reading - 3100.0) / 600.0, 0.0, 1.0);
  return ng_round_to_int(normalized * 100.0);
}

static int angular_http_write_api_live(void *context,
                                       const ng_http_request_t *request,
                                       ng_http_response_t *response) {
  angular_http_service_t *service = (angular_http_service_t *)context;
  (void)request;
  snprintf(response->body,
           sizeof(response->body),
           "{\"reading\":%d,\"percentStraight\":%d,\"angleDeg\":%.1f,\"speed\":0.0,\"accel\":0.0,\"inStep\":false,\"stepCount\":1,\"lastScore\":92.0,\"todayAverage\":92.0,\"timeSynced\":true,\"goal\":85}",
           ng_runtime_get_int(service->runtime, "reading", 0),
           ng_runtime_get_int(service->runtime, "percentStraight", 0),
           ng_runtime_get_double(service->runtime, "angleDeg", 0.0));
  return 0;
}

static int angular_http_write_api_history(void *context,
                                          const ng_http_request_t *request,
                                          ng_http_response_t *response) {
  angular_http_service_t *service = (angular_http_service_t *)context;
  (void)request;
  snprintf(response->body,
           sizeof(response->body),
           "{\"goal\":85,\"history\":[{\"timestampMs\":1712815200000,\"score\":92.0,\"shakiness\":1.2,\"uncontrolledDescent\":0.4,\"compensation\":0.3,\"durationMs\":1280.0,\"range\":%d,\"descentAvgSpeed\":0.8,\"ascentAvgSpeed\":0.9,\"oscillations\":1}],\"dailyAverages\":[{\"dayStartMs\":1712793600000,\"averageScore\":92.0,\"count\":1}]}",
           ng_runtime_get_int(service->runtime, "percentStraight", 0));
  return 0;
}

static int angular_http_write_api_time_sync(void *context,
                                            const ng_http_request_t *request,
                                            ng_http_response_t *response) {
  (void)context;
  (void)request;
  strcpy(response->body, "{\"ok\":true}");
  return 0;
}

static int angular_http_write_state(void *context,
                                    const ng_http_request_t *request,
                                    ng_http_response_t *response) {
  angular_http_service_t *service = (angular_http_service_t *)context;
  (void)request;
  angular_http_write_state_json(service->runtime, response->body, sizeof(response->body));
  return 0;
}

static int angular_http_write_value(void *context,
                                    const ng_http_request_t *request,
                                    ng_http_response_t *response) {
  angular_http_service_t *service = (angular_http_service_t *)context;
  (void)request;
  strcpy(response->content_type, "text/plain");
  snprintf(response->body, sizeof(response->body), "%d", ng_runtime_get_int(service->runtime, "reading", 0));
  return 0;
}

static int angular_http_write_reading(void *context,
                                      const ng_http_request_t *request,
                                      ng_http_response_t *response) {
  angular_http_service_t *service = (angular_http_service_t *)context;
  int percent_straight;
  int reading;
  if (ng_extract_reading_json(request->body, &reading) != 0) {
    response->status_code = 400;
    strcpy(response->body, "{\"error\":\"invalid reading json\"}");
    return 0;
  }
  percent_straight = angular_http_percent_from_reading(reading);
  ng_runtime_set_int(service->runtime, "reading", reading);
  ng_runtime_set_int(service->runtime, "percentStraight", percent_straight);
  ng_runtime_set_double(service->runtime, "angleDeg", angular_mapPercentToAngle_call(service->runtime, (double)percent_straight));
  ng_runtime_set_bool(service->runtime, "hasSignal", 1);
  angular_updateKnee_call(service->runtime, (double)percent_straight);
  angular_http_write_state_json(service->runtime, response->body, sizeof(response->body));
  return 0;
}

void angular_http_service_init(angular_http_service_t *service,
                               ng_runtime_t *runtime,
                               const char *html_page,
                               const char *css_text,
                               const char *js_text) {
  service->runtime = runtime;
  service->routes[0].method = "GET";
  service->routes[0].path = "/state";
  service->routes[0].handler = angular_http_write_state;
  service->routes[0].context = service;
  service->routes[1].method = "GET";
  service->routes[1].path = "/value";
  service->routes[1].handler = angular_http_write_value;
  service->routes[1].context = service;
  service->routes[2].method = "POST";
  service->routes[2].path = "/reading";
  service->routes[2].handler = angular_http_write_reading;
  service->routes[2].context = service;
  service->routes[3].method = "GET";
  service->routes[3].path = "/api/live";
  service->routes[3].handler = angular_http_write_api_live;
  service->routes[3].context = service;
  service->routes[4].method = "GET";
  service->routes[4].path = "/api/history";
  service->routes[4].handler = angular_http_write_api_history;
  service->routes[4].context = service;
  service->routes[5].method = "GET";
  service->routes[5].path = "/api/time-sync";
  service->routes[5].handler = angular_http_write_api_time_sync;
  service->routes[5].context = service;
  ng_http_service_init(&service->service, html_page, css_text, js_text, service->routes, 6);
}
