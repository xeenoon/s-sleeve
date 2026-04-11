#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "angular_http_service.h"
#include "angular_mapPercentToAngle.h"
#include "angular_ngOnInit.h"
#include "angular_updateKnee.h"
#include "helpers/include/net/http_server.h"
#include "helpers/include/net/http_service.h"

static char g_index_html[16384];
static char g_styles_css[16384];
static char g_app_js[16384];

static int angular_demo_load_file(const char *path, char *buffer, size_t buffer_size) {
  FILE *file = fopen(path, "rb");
  size_t bytes_read;
  if (file == NULL) {
    fprintf(stderr, "[demo] failed to open %s\n", path);
    return 1;
  }
  bytes_read = fread(buffer, 1, buffer_size - 1, file);
  if (ferror(file)) {
    fclose(file);
    fprintf(stderr, "[demo] failed to read %s\n", path);
    return 1;
  }
  buffer[bytes_read] = '\0';
  fclose(file);
  printf("[demo] loaded %s bytes=%zu\n", path, bytes_read);
  fflush(stdout);
  return 0;
}

int main(int argc, char **argv) {
  ng_runtime_t runtime;
  angular_http_service_t service;
  unsigned short port = 18080;
  int max_requests = 0;

  if (argc >= 2) {
    port = (unsigned short)atoi(argv[1]);
  }
  if (argc >= 3) {
    max_requests = atoi(argv[2]);
  }

  if (angular_demo_load_file("index.html", g_index_html, sizeof(g_index_html)) != 0 ||
      angular_demo_load_file("styles.css", g_styles_css, sizeof(g_styles_css)) != 0 ||
      angular_demo_load_file("app.js", g_app_js, sizeof(g_app_js)) != 0) {
    return 1;
  }

  angular_ngOnInit_call(&runtime);
  ng_runtime_set_int(&runtime, "reading", 3500);
  ng_runtime_set_int(&runtime, "percentStraight", 67);
  ng_runtime_set_double(&runtime, "angleDeg", angular_mapPercentToAngle_call(&runtime, 67.0));
  ng_runtime_set_bool(&runtime, "hasSignal", 1);
  ng_runtime_set_double(&runtime, "speed", 0.0);
  ng_runtime_set_double(&runtime, "lastScore", 92.0);
  ng_runtime_set_int(&runtime, "stepCount", 1);
  ng_runtime_set_double(&runtime, "todayAverage", 92.0);
  ng_runtime_set_bool(&runtime, "timeSynced", 1);
  ng_runtime_set_bool(&runtime, "inStep", 0);
  ng_runtime_set_double(&runtime, "shaky", 1.2);
  ng_runtime_set_double(&runtime, "uncontrolledDescent", 0.4);
  ng_runtime_set_double(&runtime, "compensation", 0.3);
  angular_updateKnee_call(&runtime, 67.0);
  printf("[demo] starting port=%u max_requests=%d reading=%d\n", port, max_requests, ng_runtime_get_int(&runtime, "reading", 0));
  fflush(stdout);
  angular_http_service_init(&service, &runtime, g_index_html, g_styles_css, g_app_js);
  return ng_http_server_serve(port, ng_http_service_handle, &service.service, max_requests);
}
