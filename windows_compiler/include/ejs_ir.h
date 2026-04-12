#ifndef EJS_IR_H
#define EJS_IR_H

#include <stddef.h>

typedef enum {
  NG_EJS_NODE_TEXT = 0,
  NG_EJS_NODE_EXPR,
  NG_EJS_NODE_RAW_EXPR,
  NG_EJS_NODE_IF_OPEN,
  NG_EJS_NODE_IF_CLOSE
} ng_ejs_node_kind_t;

typedef struct {
  ng_ejs_node_kind_t kind;
  char text[512];
} ng_ejs_node_t;

typedef struct {
  char name[128];
  ng_ejs_node_t nodes[512];
  size_t node_count;
} ng_ejs_template_t;

typedef struct {
  ng_ejs_template_t templates[32];
  size_t template_count;
} ng_ejs_template_set_t;

typedef enum {
  NG_LOCAL_NULL = 0,
  NG_LOCAL_BOOL,
  NG_LOCAL_INT,
  NG_LOCAL_DOUBLE,
  NG_LOCAL_STRING
} ng_local_kind_t;

typedef struct {
  const char *name;
  ng_local_kind_t kind;
  union {
    int int_value;
    double double_value;
    int bool_value;
    const char *string_value;
  } data;
} ng_local_slot_t;

typedef struct {
  ng_local_slot_t slots[64];
  size_t slot_count;
} ng_render_context_t;

void ng_ejs_template_set_init(ng_ejs_template_set_t *set);
int ejs_parser_parse_text(const char *name, const char *text, ng_ejs_template_t *out_template);
int ejs_parser_parse_file(const char *path, ng_ejs_template_t *out_template);
int ejs_parser_collect(const char *views_root, ng_ejs_template_set_t *out_set);
int ejs_codegen_emit_source(const ng_ejs_template_set_t *templates,
                            char *buffer,
                            size_t buffer_size);

#endif
