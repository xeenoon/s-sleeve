#ifndef NG_SERVER_RUNTIME_H
#define NG_SERVER_RUNTIME_H

#include <stddef.h>

#include "data/json.h"
#include "net/http_server.h"
#include "support/stringbuilder.h"

typedef struct {
  const char *name;
  const char *expr_source;
} ng_server_binding_t;

typedef enum {
  NG_TEMPLATE_NODE_TEXT = 0,
  NG_TEMPLATE_NODE_EXPR,
  NG_TEMPLATE_NODE_RAW_EXPR,
  NG_TEMPLATE_NODE_IF_OPEN,
  NG_TEMPLATE_NODE_ELSE,
  NG_TEMPLATE_NODE_END,
  NG_TEMPLATE_NODE_FOR_OPEN,
  NG_TEMPLATE_NODE_INCLUDE
} ng_template_node_kind_t;

typedef struct {
  ng_template_node_kind_t kind;
  const char *text;
} ng_template_node_t;

typedef struct {
  const char *name;
  const char *layout_name;
  const ng_template_node_t *nodes;
  size_t node_count;
} ng_template_def_t;

typedef struct ng_server_scope_entry_t {
  const char *name;
  json_data *value;
  struct ng_server_scope_entry_t *next;
} ng_server_scope_entry_t;

json_data *ng_server_json_clone(const json_data *value);
int ng_server_json_truthy(const json_data *value);
json_data *ng_server_eval_expr(const char *expr_source,
                               const ng_http_request_t *request,
                               const ng_server_binding_t *bindings,
                               size_t binding_count,
                               const ng_server_scope_entry_t *scope,
                               const json_data *model);
void ng_server_append_escaped_value(stringbuilder *builder, const json_data *value);
void ng_server_append_raw_value(stringbuilder *builder, const json_data *value);
int ng_server_render_template_response(const ng_template_def_t *templates,
                                       size_t template_count,
                                       const char *template_name,
                                       json_data *model,
                                       const ng_http_request_t *request,
                                       ng_http_response_t *response);

#endif
