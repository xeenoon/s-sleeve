#ifndef NG_HTTP_SERVER_H
#define NG_HTTP_SERVER_H

#include <stddef.h>

typedef struct {
  char method[8];
  char path[128];
  char body[16384];
  size_t body_length;
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
int ng_http_server_serve(unsigned short port,
                         ng_http_handler_t handler,
                         void *context,
                         int max_requests);

#endif
