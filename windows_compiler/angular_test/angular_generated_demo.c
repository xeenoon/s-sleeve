#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "angular_http_service.h"
#include "helpers/include/net/http_server.h"
#include "helpers/include/net/http_service.h"

static char *g_index_html = NULL;
static char *g_styles_css = NULL;
static char *g_app_js = NULL;

static int angular_demo_load_file(const char *path, char **buffer, size_t *buffer_size) {
  FILE *file = fopen(path, "rb");
  long file_size;
  size_t bytes_read;
  if (file == NULL) {
    fprintf(stderr, "[demo] failed to open %s\n", path);
    return 1;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    fprintf(stderr, "[demo] failed to seek %s\n", path);
    return 1;
  }
  file_size = ftell(file);
  if (file_size < 0) {
    fclose(file);
    fprintf(stderr, "[demo] failed to size %s\n", path);
    return 1;
  }
  if (fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    fprintf(stderr, "[demo] failed to rewind %s\n", path);
    return 1;
  }
  *buffer = (char *)malloc((size_t)file_size + 1u);
  if (*buffer == NULL) {
    fclose(file);
    fprintf(stderr, "[demo] failed to allocate %s bytes=%ld\n", path, file_size);
    return 1;
  }
  bytes_read = fread(*buffer, 1, (size_t)file_size, file);
  if (ferror(file)) {
    free(*buffer);
    *buffer = NULL;
    fclose(file);
    fprintf(stderr, "[demo] failed to read %s\n", path);
    return 1;
  }
  if (bytes_read != (size_t)file_size) {
    free(*buffer);
    *buffer = NULL;
    fclose(file);
    fprintf(stderr, "[demo] short read %s expected=%ld actual=%zu\n", path, file_size, bytes_read);
    return 1;
  }
  (*buffer)[bytes_read] = '\0';
  *buffer_size = bytes_read;
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

  size_t index_html_size = 0;
  size_t styles_css_size = 0;
  size_t app_js_size = 0;

  if (angular_demo_load_file("index.html", &g_index_html, &index_html_size) != 0 ||
      angular_demo_load_file("styles.css", &g_styles_css, &styles_css_size) != 0 ||
      angular_demo_load_file("app.js", &g_app_js, &app_js_size) != 0) {
    free(g_index_html);
    free(g_styles_css);
    free(g_app_js);
    return 1;
  }

  ng_runtime_init(&runtime);
  (void)index_html_size;
  (void)styles_css_size;
  (void)app_js_size;
  printf("[demo] starting port=%u max_requests=%d\n", port, max_requests);
  fflush(stdout);
  angular_http_service_init(&service, &runtime, g_index_html, g_styles_css, g_app_js);
  max_requests = ng_http_server_serve(port, ng_http_service_handle, &service.service, max_requests);
  free(g_index_html);
  free(g_styles_css);
  free(g_app_js);
  return max_requests;
}
