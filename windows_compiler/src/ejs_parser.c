#include "ejs_ir.h"

#include <stdio.h>
#include <string.h>

#include "file_io.h"
#include "output_fs.h"
#include "path_scan.h"

typedef struct {
  ng_ejs_template_set_t *set;
} ejs_collect_context_t;

static void ejs_trim_copy(char *buffer, size_t buffer_size, const char *start, size_t length) {
  size_t begin = 0;
  size_t end = length;

  if (buffer_size == 0) {
    return;
  }

  while (begin < length &&
         (start[begin] == ' ' || start[begin] == '\t' || start[begin] == '\r' || start[begin] == '\n')) {
    begin += 1;
  }
  while (end > begin &&
         (start[end - 1] == ' ' || start[end - 1] == '\t' || start[end - 1] == '\r' || start[end - 1] == '\n')) {
    end -= 1;
  }

  if (end - begin >= buffer_size) {
    end = begin + buffer_size - 1;
  }

  memcpy(buffer, start + begin, end - begin);
  buffer[end - begin] = '\0';
}

static int ejs_template_add_node(ng_ejs_template_t *template_file,
                                 ng_ejs_node_kind_t kind,
                                 const char *text,
                                 size_t length) {
  ng_ejs_node_t *node;

  if (template_file->node_count >= sizeof(template_file->nodes) / sizeof(template_file->nodes[0])) {
    return 1;
  }

  node = &template_file->nodes[template_file->node_count++];
  memset(node, 0, sizeof(*node));
  node->kind = kind;

  if (text != NULL) {
    ejs_trim_copy(node->text, sizeof(node->text), text, length);
  }

  return 0;
}

void ng_ejs_template_set_init(ng_ejs_template_set_t *set) {
  if (set != NULL) {
    memset(set, 0, sizeof(*set));
  }
}

int ejs_parser_parse_text(const char *name, const char *text, ng_ejs_template_t *out_template) {
  size_t index = 0;
  size_t text_start = 0;

  if (name == NULL || text == NULL || out_template == NULL) {
    return 1;
  }

  memset(out_template, 0, sizeof(*out_template));
  snprintf(out_template->name, sizeof(out_template->name), "%s", name);

  while (text[index] != '\0') {
    if (text[index] == '<' && text[index + 1] == '%') {
      size_t tag_start = index;
      size_t expr_start;
      size_t expr_end;
      ng_ejs_node_kind_t kind = NG_EJS_NODE_TEXT;

      if (tag_start > text_start &&
          ejs_template_add_node(out_template, NG_EJS_NODE_TEXT, text + text_start, tag_start - text_start) != 0) {
        return 1;
      }

      index += 2;
      if (text[index] == '=') {
        kind = NG_EJS_NODE_EXPR;
        index += 1;
      } else if (text[index] == '-') {
        kind = NG_EJS_NODE_RAW_EXPR;
        index += 1;
      }

      expr_start = index;
      while (text[index] != '\0' && !(text[index] == '%' && text[index + 1] == '>')) {
        index += 1;
      }
      if (text[index] == '\0') {
        return 1;
      }
      expr_end = index;

      if (kind == NG_EJS_NODE_TEXT) {
        char statement[512];
        ejs_trim_copy(statement, sizeof(statement), text + expr_start, expr_end - expr_start);
        if (strncmp(statement, "if", 2) == 0) {
          const char *open = strchr(statement, '(');
          const char *close = strrchr(statement, ')');
          if (open == NULL || close == NULL || close <= open) {
            return 1;
          }
          if (ejs_template_add_node(out_template,
                                    NG_EJS_NODE_IF_OPEN,
                                    open + 1,
                                    (size_t)(close - (open + 1))) != 0) {
            return 1;
          }
        } else if (strcmp(statement, "}") == 0) {
          if (ejs_template_add_node(out_template, NG_EJS_NODE_IF_CLOSE, "", 0) != 0) {
            return 1;
          }
        } else {
          return 1;
        }
      } else if (ejs_template_add_node(out_template, kind, text + expr_start, expr_end - expr_start) != 0) {
        return 1;
      }

      index += 2;
      text_start = index;
      continue;
    }
    index += 1;
  }

  if (index > text_start &&
      ejs_template_add_node(out_template, NG_EJS_NODE_TEXT, text + text_start, index - text_start) != 0) {
    return 1;
  }

  return 0;
}

static void ejs_template_name_from_path(char *buffer, size_t buffer_size, const char *path) {
  const char *name = strrchr(path, '\\');
  const char *dot;

  if (name == NULL) {
    name = path;
  } else {
    name += 1;
  }

  snprintf(buffer, buffer_size, "%s", name);
  dot = strrchr(buffer, '.');
  if (dot != NULL) {
    buffer[dot - buffer] = '\0';
  }
}

int ejs_parser_parse_file(const char *path, ng_ejs_template_t *out_template) {
  file_buffer_t buffer;
  char name[128];
  int result;

  if (file_read_all(path, &buffer) != 0) {
    return 1;
  }

  ejs_template_name_from_path(name, sizeof(name), path);
  result = ejs_parser_parse_text(name, buffer.data, out_template);
  file_buffer_free(&buffer);
  return result;
}

static int ejs_collect_file(const char *path, void *context) {
  ejs_collect_context_t *collect = (ejs_collect_context_t *)context;
  ng_ejs_template_t *template_file;

  if (strstr(path, ".ejs") == NULL) {
    return 0;
  }
  if (collect->set->template_count >= sizeof(collect->set->templates) / sizeof(collect->set->templates[0])) {
    return 1;
  }

  template_file = &collect->set->templates[collect->set->template_count];
  if (ejs_parser_parse_file(path, template_file) != 0) {
    return 1;
  }

  collect->set->template_count += 1;
  return 0;
}

int ejs_parser_collect(const char *views_root, ng_ejs_template_set_t *out_set) {
  ejs_collect_context_t context;

  if (views_root == NULL || out_set == NULL) {
    return 1;
  }

  ng_ejs_template_set_init(out_set);
  if (!output_fs_file_exists(views_root)) {
    return 0;
  }

  context.set = out_set;
  return path_scan_directory(views_root, ejs_collect_file, &context);
}
