#ifndef JS_CODEGEN_H
#define JS_CODEGEN_H

#include <stddef.h>

#include "ast.h"

typedef enum {
  JS_TYPE_UNKNOWN = 0,
  JS_TYPE_INT,
  JS_TYPE_DOUBLE,
  JS_TYPE_BOOL,
  JS_TYPE_STRING,
  JS_TYPE_VOID
} js_type_t;

typedef struct {
  js_type_t return_type;
  char prototype[256];
  char definition[8192];
  int supported;
} js_codegen_result_t;

int js_codegen_compile_method(const ast_component_file_t *component,
                              const ast_member_t *member,
                              js_codegen_result_t *out_result);
const char *js_type_name(js_type_t type);

#endif
