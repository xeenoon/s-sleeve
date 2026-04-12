#ifndef NG_HTTP_SERVER_H
#define NG_HTTP_SERVER_H

#include <stddef.h>

#define NG_HTTP_MAX_PAIRS 16

typedef struct {
  char key[64];
  char value[256];
} ng_http_pair_t;

typedef struct {
  char method[8];
  char raw_path[128];
  char path[128];
  char content_type[64];
  char body[16384];
  size_t body_length;
  ng_http_pair_t query_pairs[NG_HTTP_MAX_PAIRS];
  size_t query_count;
  ng_http_pair_t body_pairs[NG_HTTP_MAX_PAIRS];
  size_t body_count;
  ng_http_pair_t route_params[NG_HTTP_MAX_PAIRS];
  size_t route_param_count;
  ng_http_pair_t headers[NG_HTTP_MAX_PAIRS];
  size_t header_count;
} ng_http_request_t;

typedef struct {
  int status_code;
  char content_type[64];
  char body[4096];
  char *dynamic_body;
  size_t dynamic_body_length;
} ng_http_response_t;

typedef int (*ng_http_handler_t)(void *context,
                                 const ng_http_request_t *request,
                                 ng_http_response_t *response);

void ng_http_response_init(ng_http_response_t *response);
void ng_http_response_dispose(ng_http_response_t *response);
const char *ng_http_response_body(const ng_http_response_t *response);
int ng_http_response_set_text(ng_http_response_t *response, const char *text);
int ng_http_response_set_format(ng_http_response_t *response, const char *format, ...);
const char *ng_http_request_query(const ng_http_request_t *request, const char *key);
const char *ng_http_request_body_field(const ng_http_request_t *request, const char *key);
const char *ng_http_request_route_param(const ng_http_request_t *request, const char *key);
const char *ng_http_request_header(const ng_http_request_t *request, const char *key);
void ng_http_request_set_route_param(ng_http_request_t *request, const char *key, const char *value);
void ng_http_request_clear_route_params(ng_http_request_t *request);
int ng_http_server_serve(unsigned short port,
                         ng_http_handler_t handler,
                         void *context,
                         int max_requests);

#endif
