#include "ejs_ir.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "file_io.h"
#include "output_fs.h"
#include "path_scan.h"

typedef struct {
  ng_ejs_template_set_t *set;
  const char *views_root;
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

static void ejs_normalize_name(char *buffer) {
  size_t index;
  for (index = 0; buffer[index] != '\0'; ++index) {
    if (buffer[index] == '\\') {
      buffer[index] = '/';
    }
  }
}

static int ejs_copy_string_argument(char *buffer, size_t buffer_size, const char *text) {
  const char *open;
  const char *close;

  if (buffer_size == 0) {
    return 1;
  }

  open = strchr(text, '(');
  close = strrchr(text, ')');
  if (open == NULL || close == NULL || close <= open + 1) {
    return 1;
  }

  open += 1;
  while (*open == ' ' || *open == '\t') {
    open += 1;
  }
  if (*open != '\'' && *open != '"') {
    return 1;
  }

  {
    char quote = *open++;
    size_t cursor = 0;
    while (open < close && *open != quote && cursor + 1 < buffer_size) {
      if (*open == '\\' && open + 1 < close) {
        open += 1;
      }
      buffer[cursor++] = *open++;
    }
    buffer[cursor] = '\0';
    if (*open != quote) {
      return 1;
    }
  }

  ejs_normalize_name(buffer);
  return 0;
}

static int ejs_parse_include(char *buffer, size_t buffer_size, const char *statement) {
  const char *open;
  const char *comma;
  const char *close;
  char name[128];

  if (ejs_copy_string_argument(name, sizeof(name), statement) != 0) {
    return 1;
  }

  open = strchr(statement, '(');
  close = strrchr(statement, ')');
  comma = open != NULL && close != NULL ? strchr(open, ',') : NULL;
  if (comma != NULL && comma < close) {
    char expr[384];
    ejs_trim_copy(expr, sizeof(expr), comma + 1, (size_t)(close - (comma + 1)));
    snprintf(buffer, buffer_size, "%s|%s", name, expr);
  } else {
    snprintf(buffer, buffer_size, "%s", name);
  }
  return 0;
}

static int ejs_parse_for_statement(char *buffer, size_t buffer_size, const char *statement) {
  const char *open = strchr(statement, '(');
  const char *close = strrchr(statement, ')');
  const char *of_keyword;
  char item_name[64];
  char expr[384];

  if (open == NULL || close == NULL || close <= open) {
    return 1;
  }

  of_keyword = strstr(open + 1, " of ");
  if (of_keyword == NULL || of_keyword >= close) {
    return 1;
  }

  ejs_trim_copy(item_name, sizeof(item_name), open + 1, (size_t)(of_keyword - (open + 1)));
  ejs_trim_copy(expr, sizeof(expr), of_keyword + 4, (size_t)(close - (of_keyword + 4)));
  snprintf(buffer, buffer_size, "%s|%s", item_name, expr);
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
  ejs_normalize_name(out_template->name);

  while (text[index] != '\0') {
    if (text[index] == '<' && text[index + 1] == '%') {
      size_t tag_start = index;
      size_t expr_start;
      size_t expr_end;
      ng_ejs_node_kind_t kind = NG_EJS_NODE_TEXT;
      char statement[512];

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
      } else if (text[index] == '#') {
        kind = NG_EJS_NODE_TEXT;
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

      if (kind == NG_EJS_NODE_TEXT && text[expr_start - 1] == '#') {
        index += 2;
        text_start = index;
        continue;
      }

      if (kind == NG_EJS_NODE_EXPR || kind == NG_EJS_NODE_RAW_EXPR) {
        if (ejs_template_add_node(out_template, kind, text + expr_start, expr_end - expr_start) != 0) {
          return 1;
        }
      } else {
        ejs_trim_copy(statement, sizeof(statement), text + expr_start, expr_end - expr_start);
        if (statement[0] == '\0') {
          index += 2;
          text_start = index;
          continue;
        }
        if (strncmp(statement, "if", 2) == 0) {
          const char *open = strchr(statement, '(');
          const char *close = strrchr(statement, ')');
          if (open == NULL || close == NULL || close <= open) {
            return 1;
          }
          if (ejs_template_add_node(out_template, NG_EJS_NODE_IF_OPEN, open + 1, (size_t)(close - (open + 1))) != 0) {
            return 1;
          }
        } else if (strncmp(statement, "} else", 6) == 0 || strcmp(statement, "else") == 0) {
          if (ejs_template_add_node(out_template, NG_EJS_NODE_ELSE, "", 0) != 0) {
            return 1;
          }
        } else if (strncmp(statement, "for", 3) == 0) {
          char loop_text[512];
          if (ejs_parse_for_statement(loop_text, sizeof(loop_text), statement) != 0 ||
              ejs_template_add_node(out_template, NG_EJS_NODE_FOR_OPEN, loop_text, strlen(loop_text)) != 0) {
            return 1;
          }
        } else if (strncmp(statement, "include", 7) == 0) {
          char include_text[512];
          if (ejs_parse_include(include_text, sizeof(include_text), statement) != 0 ||
              ejs_template_add_node(out_template, NG_EJS_NODE_INCLUDE, include_text, strlen(include_text)) != 0) {
            return 1;
          }
        } else if (strncmp(statement, "layout", 6) == 0) {
          if (ejs_copy_string_argument(out_template->layout_name, sizeof(out_template->layout_name), statement) != 0) {
            return 1;
          }
        } else if (strcmp(statement, "}") == 0) {
          if (ejs_template_add_node(out_template, NG_EJS_NODE_END, "", 0) != 0) {
            return 1;
          }
        } else {
          return 1;
        }
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

static void ejs_template_name_from_paths(char *buffer,
                                         size_t buffer_size,
                                         const char *views_root,
                                         const char *path) {
  const char *relative = path;
  size_t root_length;
  char *dot;

  if (buffer_size == 0) {
    return;
  }

  root_length = strlen(views_root);
  if (_strnicmp(path, views_root, root_length) == 0) {
    relative = path + root_length;
    while (*relative == '\\' || *relative == '/') {
      relative += 1;
    }
  } else {
    const char *name = strrchr(path, '\\');
    relative = name != NULL ? name + 1 : path;
  }

  snprintf(buffer, buffer_size, "%s", relative);
  ejs_normalize_name(buffer);
  dot = strrchr(buffer, '.');
  if (dot != NULL) {
    *dot = '\0';
  }
}

int ejs_parser_parse_file(const char *path, ng_ejs_template_t *out_template) {
  file_buffer_t buffer;
  char name[128];
  int result;

  if (file_read_all(path, &buffer) != 0) {
    return 1;
  }

  ejs_template_name_from_paths(name, sizeof(name), "", path);
  result = ejs_parser_parse_text(name, buffer.data, out_template);
  file_buffer_free(&buffer);
  return result;
}

static int ejs_collect_file(const char *path, void *context) {
  ejs_collect_context_t *collect = (ejs_collect_context_t *)context;
  ng_ejs_template_t *template_file;
  file_buffer_t buffer;
  char name[128];
  int result;

  if (strstr(path, ".ejs") == NULL) {
    return 0;
  }
  if (collect->set->template_count >= sizeof(collect->set->templates) / sizeof(collect->set->templates[0])) {
    return 1;
  }
  if (file_read_all(path, &buffer) != 0) {
    return 1;
  }

  ejs_template_name_from_paths(name, sizeof(name), collect->views_root, path);
  template_file = &collect->set->templates[collect->set->template_count];
  result = ejs_parser_parse_text(name, buffer.data, template_file);
  file_buffer_free(&buffer);
  if (result != 0) {
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
  context.views_root = views_root;
  return path_scan_directory(views_root, ejs_collect_file, &context);
}
