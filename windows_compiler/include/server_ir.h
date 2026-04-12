#ifndef SERVER_IR_H
#define SERVER_IR_H

#include <stddef.h>

#include "ejs_ir.h"

typedef enum {
  NG_SERVER_METHOD_GET = 0,
  NG_SERVER_METHOD_POST = 1
} ng_server_method_t;

typedef enum {
  NG_RESPONSE_JSON = 0,
  NG_RESPONSE_TEXT,
  NG_RESPONSE_RENDER
} ng_server_response_kind_t;

typedef struct {
  char name[64];
  char expr_source[4096];
} ng_server_binding_ir_t;

typedef struct {
  ng_server_method_t method;
  char path[256];
  ng_server_response_kind_t response_kind;
  char response_expr[4096];
  char template_name[128];
  char model_expr[4096];
  ng_server_binding_ir_t bindings[32];
  size_t binding_count;
  int status_code;
} ng_server_route_ir_t;

typedef struct {
  ng_server_route_ir_t routes[32];
  size_t route_count;
} ng_server_route_set_t;

typedef struct {
  char support_source[65536];
  char route_source[32768];
  char route_init[16384];
  size_t route_count;
} ng_server_codegen_result_t;

void ng_server_route_set_init(ng_server_route_set_t *set);
int server_parser_parse_source(const char *source, ng_server_route_set_t *out_set);
int server_parser_parse_file(const char *path, ng_server_route_set_t *out_set);
int server_parser_collect(const char *server_root, ng_server_route_set_t *out_set);
int server_codegen_emit(const ng_server_route_set_t *routes,
                        const ng_ejs_template_set_t *templates,
                        size_t route_offset,
                        ng_server_codegen_result_t *out_result);

#endif
