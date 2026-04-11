#include "ast.h"

#include <string.h>

#include "log.h"

void ast_file_init(ast_file_t *file, parser_file_kind_t kind) {
  memset(file, 0, sizeof(*file));
  file->kind = kind;
}

void ast_file_print(const ast_file_t *file) {
  size_t index;

  switch (file->kind) {
    case PARSER_FILE_COMPONENT:
      log_printf("AST component decorator=%s class=%s members=%zu",
                 file->data.component.has_component_decorator ? "yes" : "no",
                 file->data.component.class_name[0] != '\0' ? file->data.component.class_name : "<none>",
                 file->data.component.member_count);
      if (file->data.component.member_count > 0) {
        log_printf(" [");
        for (index = 0; index < file->data.component.member_count; ++index) {
          if (index > 0) {
            log_printf(", ");
          }
          log_printf("%s:%s%s",
                     file->data.component.members[index].name,
                     file->data.component.members[index].kind == AST_MEMBER_METHOD ? "method" : "field",
                     file->data.component.members[index].is_observable ? ":observable" : "");
        }
        log_printf("]");
      }
      log_putc('\n');
      break;
    case PARSER_FILE_TEMPLATE:
      log_printf("AST template elements=%zu interpolations=%zu attr_bindings=%zu\n",
                 file->data.template_file.element_count,
                 file->data.template_file.interpolation_count,
                 file->data.template_file.attribute_binding_count);
      break;
    case PARSER_FILE_STYLE:
      log_printf("AST style rules=%zu balanced=%s max_depth=%zu\n",
                 file->data.style.rule_count,
                 file->data.style.has_balanced_blocks ? "yes" : "no",
                 file->data.style.max_block_depth);
      break;
    case PARSER_FILE_UNKNOWN:
      log_printf("AST unknown\n");
      break;
  }
}
