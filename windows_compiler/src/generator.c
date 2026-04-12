#include "generator.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "ejs_ir.h"
#include "file_io.h"
#include "js_codegen.h"
#include "log.h"
#include "output_fs.h"
#include "path_scan.h"
#include "server_ir.h"

typedef struct {
  char runtime_category[64];
  char storage_type[64];
  char processing_notes[256];
  int requires_external_fetch;
} generator_member_spec_t;

typedef enum {
  GENERATOR_OBSERVABLE_UNKNOWN = 0,
  GENERATOR_OBSERVABLE_STATE,
  GENERATOR_OBSERVABLE_POLL,
  GENERATOR_OBSERVABLE_POST
} generator_observable_kind_t;

typedef enum {
  GENERATOR_PIPE_UNKNOWN = 0,
  GENERATOR_PIPE_PROP,
  GENERATOR_PIPE_MAP,
  GENERATOR_PIPE_TAP,
  GENERATOR_PIPE_REDUCE,
  GENERATOR_PIPE_EFFECT_CLASS,
  GENERATOR_PIPE_EFFECT_CLASS_AT,
  GENERATOR_PIPE_EFFECT_STAGGER_CLASS,
  GENERATOR_PIPE_EFFECT_STYLE_VAR
} generator_pipe_kind_t;

#define GENERATOR_MAX_PIPE_ARGS 8

typedef struct {
  generator_pipe_kind_t kind;
  char argument[128];
  char arguments[GENERATOR_MAX_PIPE_ARGS][128];
  size_t argument_count;
} generator_pipe_op_t;

typedef struct {
  char name[128];
  char safe_name[128];
  generator_observable_kind_t kind;
  char path[256];
  char seed[128];
  int interval_ms;
  generator_pipe_op_t pipe_ops[8];
  size_t pipe_count;
} generator_observable_spec_t;

#define GENERATOR_MAX_OBSERVABLES 16
#define GENERATOR_MAX_ROUTES 32

typedef struct {
  char method[16];
  char path[256];
  char content_type[64];
  file_buffer_t body;
} generator_route_asset_t;

typedef struct {
  const char *routes_root;
  generator_route_asset_t routes[GENERATOR_MAX_ROUTES];
  size_t route_count;
} generator_route_collection_t;

typedef struct {
  ng_server_route_set_t routes;
  ng_ejs_template_set_t templates;
} generator_backend_assets_t;

typedef enum {
  GENERATOR_FIELD_VALUE_UNKNOWN = 0,
  GENERATOR_FIELD_VALUE_INT,
  GENERATOR_FIELD_VALUE_DOUBLE,
  GENERATOR_FIELD_VALUE_BOOL,
  GENERATOR_FIELD_VALUE_STRING
} generator_field_value_kind_t;

static const char *g_helper_directories[] = {
    "helpers",
    "helpers\\include",
    "helpers\\include\\data",
    "helpers\\include\\format",
    "helpers\\include\\io",
    "helpers\\include\\math",
    "helpers\\include\\net",
    "helpers\\include\\runtime",
    "helpers\\include\\support",
    "helpers\\src",
    "helpers\\src\\data",
    "helpers\\src\\format",
    "helpers\\src\\io",
    "helpers\\src\\math",
    "helpers\\src\\net",
    "helpers\\src\\runtime",
    "helpers\\src\\support"};

static const char *g_helper_files[] = {
    "include\\data\\json.h",
    "include\\data\\json_utils.h",
    "include\\format\\number_format.h",
    "include\\io\\fetch_runtime.h",
    "include\\math\\ng_math.h",
    "include\\math\\number_utils.h",
    "include\\net\\http_server.h",
    "include\\net\\http_service.h",
    "include\\runtime\\app_runtime.h",
    "include\\runtime\\observable.h",
    "include\\runtime\\server_runtime.h",
    "include\\support\\list.h",
    "include\\support\\stringbuilder.h",
    "src\\data\\json.c",
    "src\\data\\json_utils.c",
    "src\\format\\number_format.c",
    "src\\io\\fetch_runtime.c",
    "src\\math\\ng_math.c",
    "src\\math\\number_utils.c",
    "src\\net\\http_server.c",
    "src\\net\\http_service.c",
    "src\\runtime\\app_runtime.c",
    "src\\runtime\\observable.c",
    "src\\runtime\\server_runtime.c",
    "src\\support\\stringbuilder.c"};

static const char *g_generated_demo_files[] = {
    "angular_http_service.h",
    "angular_http_service.c",
    "angular_generated_demo.c",
    "index.html",
    "styles.css",
    "app.js",
    "Makefile"};

static void generator_build_path(char *buffer,
                                 size_t buffer_size,
                                 const char *directory,
                                 const char *filename) {
  snprintf(buffer, buffer_size, "%s\\%s", directory, filename);
  LOG_TRACE("generator_build_path directory=%s filename=%s path=%s\n", directory, filename, buffer);
}

static void generator_escape_c_string(char *buffer, size_t buffer_size, const char *text);

static int generator_prepare_helper_directories(const char *output_dir) {
  size_t index;
  char path[512];

  for (index = 0; index < sizeof(g_helper_directories) / sizeof(g_helper_directories[0]); ++index) {
    generator_build_path(path, sizeof(path), output_dir, g_helper_directories[index]);
    if (output_fs_create_directory(path) != 0) {
      return 1;
    }
  }

  return 0;
}

static int generator_write_text_asset(const char *output_dir,
                                      const char *filename,
                                      const char *text) {
  char path[512];

  generator_build_path(path, sizeof(path), output_dir, filename);
  LOG_TRACE("generator_write_text_asset filename=%s bytes=%zu\n",
            filename,
            text != NULL ? strlen(text) : 0u);
  return output_fs_write_text(path, text != NULL ? text : "");
}

static void generator_make_guard(char *buffer, size_t buffer_size, const char *name) {
  size_t index;
  size_t cursor = 0;
  cursor += (size_t)snprintf(buffer + cursor, buffer_size - cursor, "ANGULAR_");
  for (index = 0; name[index] != '\0' && cursor + 3 < buffer_size; ++index) {
    char ch = name[index];
    if (ch >= 'a' && ch <= 'z') {
      buffer[cursor++] = (char)(ch - ('a' - 'A'));
    } else if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
      buffer[cursor++] = ch;
    } else {
      buffer[cursor++] = '_';
    }
  }
  cursor += (size_t)snprintf(buffer + cursor, buffer_size - cursor, "_H");
}

static void generator_make_safe_name(char *buffer, size_t buffer_size, const char *name) {
  size_t index;
  size_t cursor = 0;

  if (buffer_size == 0) {
    return;
  }

  for (index = 0; name[index] != '\0' && cursor + 1 < buffer_size; ++index) {
    char ch = name[index];
    if ((ch >= 'a' && ch <= 'z') ||
        (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') ||
        ch == '_') {
      buffer[cursor++] = ch;
    } else {
      buffer[cursor++] = '_';
    }
  }

  buffer[cursor] = '\0';
}

static void generator_trim_copy(char *buffer, size_t buffer_size, const char *start, size_t length) {
  size_t begin = 0;
  size_t end = length;

  while (begin < length && (start[begin] == ' ' || start[begin] == '\t' || start[begin] == '\r' || start[begin] == '\n')) {
    begin += 1;
  }
  while (end > begin &&
         (start[end - 1] == ' ' || start[end - 1] == '\t' || start[end - 1] == '\r' || start[end - 1] == '\n')) {
    end -= 1;
  }

  if (buffer_size == 0) {
    return;
  }

  if (end - begin >= buffer_size) {
    end = begin + buffer_size - 1;
  }

  memcpy(buffer, start + begin, end - begin);
  buffer[end - begin] = '\0';
}

static int generator_parse_quoted_argument(const char *text,
                                           const char *prefix,
                                           char *buffer,
                                           size_t buffer_size) {
  const char *start = strstr(text, prefix);
  const char *quote;
  const char *end;

  if (start == NULL) {
    return 1;
  }

  start += strlen(prefix);
  quote = strchr(start, '\'');
  if (quote == NULL) {
    return 1;
  }
  quote += 1;
  end = strchr(quote, '\'');
  if (end == NULL) {
    return 1;
  }

  generator_trim_copy(buffer, buffer_size, quote, (size_t)(end - quote));
  return 0;
}

static void generator_strip_wrapping_quotes(char *buffer) {
  size_t length;
  if (buffer == NULL) {
    return;
  }
  length = strlen(buffer);
  if (length >= 2 && buffer[0] == '\'' && buffer[length - 1] == '\'') {
    memmove(buffer, buffer + 1, length - 2);
    buffer[length - 2] = '\0';
  }
}

static int generator_parse_call_arguments(const char *text,
                                          const char *prefix,
                                          char arguments[GENERATOR_MAX_PIPE_ARGS][128],
                                          size_t *argument_count) {
  const char *start = strstr(text, prefix);
  const char *cursor;
  char segment[256];
  size_t segment_length = 0;
  int depth = 1;
  int in_quote = 0;
  size_t count = 0;

  if (argument_count == NULL) {
    return 1;
  }
  *argument_count = 0;
  if (start == NULL) {
    return 1;
  }

  start += strlen(prefix);
  cursor = start;

  while (*cursor != '\0' && depth > 0) {
    char ch = *cursor++;
    if (ch == '\'') {
      in_quote = !in_quote;
    } else if (!in_quote && ch == '(') {
      depth += 1;
    } else if (!in_quote && ch == ')') {
      depth -= 1;
      if (depth == 0) {
        if (segment_length > 0 && count < GENERATOR_MAX_PIPE_ARGS) {
          segment[segment_length] = '\0';
          generator_trim_copy(arguments[count], 128, segment, strlen(segment));
          generator_strip_wrapping_quotes(arguments[count]);
          count += 1;
        }
        break;
      }
    }

    if (!in_quote && depth == 1 && ch == ',') {
      if (segment_length > 0 && count < GENERATOR_MAX_PIPE_ARGS) {
        segment[segment_length] = '\0';
        generator_trim_copy(arguments[count], 128, segment, strlen(segment));
        generator_strip_wrapping_quotes(arguments[count]);
        count += 1;
      }
      segment_length = 0;
      continue;
    }

    if (depth > 0 && segment_length + 1 < sizeof(segment)) {
      segment[segment_length++] = ch;
    }
  }

  *argument_count = count;
  LOG_TRACE("generator_parse_call_arguments prefix=%s count=%zu text=%s\n", prefix, count, text);
  return count > 0 ? 0 : 1;
}

static int generator_parse_numeric_argument_after_quote(const char *text,
                                                        const char *prefix,
                                                        int *out_value) {
  const char *start = strstr(text, prefix);
  const char *cursor;
  int depth = 1;
  int seen_quote = 0;
  char number_buffer[32];
  size_t number_length = 0;

  if (out_value == NULL) {
    return 1;
  }
  if (start == NULL) {
    return 1;
  }

  start += strlen(prefix);
  cursor = start;

  while (*cursor != '\0' && depth > 0) {
    char ch = *cursor++;
    if (ch == '\'') {
      seen_quote = !seen_quote;
      continue;
    }
    if (seen_quote) {
      continue;
    }
    if (ch == '(') {
      depth += 1;
      continue;
    }
    if (ch == ')') {
      depth -= 1;
      continue;
    }
    if (depth == 1 && ch == ',') {
      while (*cursor == ' ' || *cursor == '\t') {
        cursor += 1;
      }
      while ((*cursor >= '0' && *cursor <= '9') && number_length + 1 < sizeof(number_buffer)) {
        number_buffer[number_length++] = *cursor++;
      }
      number_buffer[number_length] = '\0';
      if (number_length == 0) {
        return 1;
      }
      *out_value = atoi(number_buffer);
      LOG_TRACE("generator_parse_numeric_argument_after_quote prefix=%s value=%d text=%s\n",
                prefix,
                *out_value,
                text);
      return 0;
    }
  }

  return 1;
}

static void generator_strip_trailing_dollar(char *buffer, size_t buffer_size, const char *name) {
  size_t length;
  if (buffer_size == 0) {
    return;
  }
  snprintf(buffer, buffer_size, "%s", name != NULL ? name : "");
  length = strlen(buffer);
  if (length > 0 && buffer[length - 1] == '$') {
    buffer[length - 1] = '\0';
  }
}

static int generator_parse_pipe_segment(const char *segment, generator_pipe_op_t *op) {
  memset(op, 0, sizeof(*op));

  if (strstr(segment, "rx.prop(") != NULL) {
    op->kind = GENERATOR_PIPE_PROP;
    if (generator_parse_call_arguments(segment, "rx.prop(", op->arguments, &op->argument_count) != 0) {
      return 1;
    }
    snprintf(op->argument, sizeof(op->argument), "%s", op->arguments[0]);
    return 0;
  }
  if (strstr(segment, "rx.map(") != NULL) {
    op->kind = GENERATOR_PIPE_MAP;
    if (generator_parse_call_arguments(segment, "rx.map(", op->arguments, &op->argument_count) != 0) {
      return 1;
    }
    snprintf(op->argument, sizeof(op->argument), "%s", op->arguments[0]);
    return 0;
  }
  if (strstr(segment, "rx.tap(") != NULL) {
    op->kind = GENERATOR_PIPE_TAP;
    if (generator_parse_call_arguments(segment, "rx.tap(", op->arguments, &op->argument_count) != 0) {
      return 1;
    }
    snprintf(op->argument, sizeof(op->argument), "%s", op->arguments[0]);
    return 0;
  }
  if (strstr(segment, "rx.reduce(") != NULL) {
    op->kind = GENERATOR_PIPE_REDUCE;
    if (generator_parse_call_arguments(segment, "rx.reduce(", op->arguments, &op->argument_count) != 0) {
      return 1;
    }
    snprintf(op->argument, sizeof(op->argument), "%s", op->arguments[0]);
    return 0;
  }
  if (strstr(segment, "rx.effectClassAt(") != NULL) {
    op->kind = GENERATOR_PIPE_EFFECT_CLASS_AT;
    if (generator_parse_call_arguments(segment, "rx.effectClassAt(", op->arguments, &op->argument_count) != 0) {
      return 1;
    }
    snprintf(op->argument, sizeof(op->argument), "%s", op->argument_count > 1 ? op->arguments[1] : "");
    return 0;
  }
  if (strstr(segment, "rx.effectClass(") != NULL) {
    op->kind = GENERATOR_PIPE_EFFECT_CLASS;
    if (generator_parse_call_arguments(segment, "rx.effectClass(", op->arguments, &op->argument_count) != 0) {
      return 1;
    }
    snprintf(op->argument, sizeof(op->argument), "%s", op->argument_count > 1 ? op->arguments[1] : "");
    return 0;
  }
  if (strstr(segment, "rx.effectStaggerClass(") != NULL) {
    op->kind = GENERATOR_PIPE_EFFECT_STAGGER_CLASS;
    if (generator_parse_call_arguments(segment, "rx.effectStaggerClass(", op->arguments, &op->argument_count) != 0) {
      return 1;
    }
    snprintf(op->argument, sizeof(op->argument), "%s", op->argument_count > 1 ? op->arguments[1] : "");
    return 0;
  }
  if (strstr(segment, "rx.effectStyleVar(") != NULL) {
    op->kind = GENERATOR_PIPE_EFFECT_STYLE_VAR;
    if (generator_parse_call_arguments(segment, "rx.effectStyleVar(", op->arguments, &op->argument_count) != 0) {
      return 1;
    }
    snprintf(op->argument, sizeof(op->argument), "%s", op->argument_count > 1 ? op->arguments[1] : "");
    return 0;
  }

  return 1;
}

static void generator_parse_pipe_chain(const char *initializer, generator_observable_spec_t *spec) {
  const char *pipe_start = strstr(initializer, ".pipe(");
  const char *cursor;
  int depth = 1;
  char segment[256];
  char trimmed[256];
  size_t segment_length = 0;

  spec->pipe_count = 0;
  if (pipe_start == NULL) {
    return;
  }

  cursor = pipe_start + strlen(".pipe(");
  while (*cursor != '\0' && depth > 0) {
    char ch = *cursor++;
    if (ch == '(') {
      depth += 1;
    } else if (ch == ')') {
      depth -= 1;
      if (depth == 0) {
        if (segment_length > 0 && spec->pipe_count < sizeof(spec->pipe_ops) / sizeof(spec->pipe_ops[0])) {
          segment[segment_length] = '\0';
          generator_trim_copy(trimmed, sizeof(trimmed), segment, strlen(segment));
          if (generator_parse_pipe_segment(trimmed, &spec->pipe_ops[spec->pipe_count]) == 0) {
            LOG_TRACE("generator pipe member=%s index=%zu kind=%d arg=%s argc=%zu\n",
                      spec->name,
                      spec->pipe_count,
                      (int)spec->pipe_ops[spec->pipe_count].kind,
                      spec->pipe_ops[spec->pipe_count].argument,
                      spec->pipe_ops[spec->pipe_count].argument_count);
            spec->pipe_count += 1;
          }
        }
        break;
      }
    }

    if (depth == 1 && ch == ',') {
      if (segment_length > 0 && spec->pipe_count < sizeof(spec->pipe_ops) / sizeof(spec->pipe_ops[0])) {
        segment[segment_length] = '\0';
        generator_trim_copy(trimmed, sizeof(trimmed), segment, strlen(segment));
        if (generator_parse_pipe_segment(trimmed, &spec->pipe_ops[spec->pipe_count]) == 0) {
          LOG_TRACE("generator pipe member=%s index=%zu kind=%d arg=%s argc=%zu\n",
                    spec->name,
                    spec->pipe_count,
                    (int)spec->pipe_ops[spec->pipe_count].kind,
                    spec->pipe_ops[spec->pipe_count].argument,
                    spec->pipe_ops[spec->pipe_count].argument_count);
          spec->pipe_count += 1;
        }
      }
      segment_length = 0;
      continue;
    }

    if (segment_length + 1 < sizeof(segment)) {
      segment[segment_length++] = ch;
    }
  }
}

static int generator_parse_observable_spec(const ast_member_t *member, generator_observable_spec_t *spec) {
  memset(spec, 0, sizeof(*spec));
  snprintf(spec->name, sizeof(spec->name), "%s", member->name);
  generator_make_safe_name(spec->safe_name, sizeof(spec->safe_name), member->name);
  spec->interval_ms = 0;

  if (!member->is_observable) {
    return 1;
  }

  if (strstr(member->initializer, "rx.state(") != NULL) {
    spec->kind = GENERATOR_OBSERVABLE_STATE;
    if (generator_parse_quoted_argument(member->initializer, "rx.state(", spec->seed, sizeof(spec->seed)) != 0) {
      snprintf(spec->seed, sizeof(spec->seed), "live");
    }
    generator_parse_pipe_chain(member->initializer, spec);
    return 0;
  }

  if (strstr(member->initializer, "rx.poll(") != NULL) {
    spec->kind = GENERATOR_OBSERVABLE_POLL;
    if (generator_parse_quoted_argument(member->initializer, "rx.poll(", spec->path, sizeof(spec->path)) != 0) {
      return 1;
    }
    generator_parse_numeric_argument_after_quote(member->initializer, "rx.poll(", &spec->interval_ms);
    if (spec->interval_ms <= 0) {
      spec->interval_ms = 1000;
    }
    generator_parse_pipe_chain(member->initializer, spec);
    return 0;
  }

  if (strstr(member->initializer, "rx.post(") != NULL) {
    spec->kind = GENERATOR_OBSERVABLE_POST;
    if (generator_parse_quoted_argument(member->initializer, "rx.post(", spec->path, sizeof(spec->path)) != 0) {
      return 1;
    }
    generator_parse_pipe_chain(member->initializer, spec);
    return 0;
  }

  return 1;
}

static size_t __attribute__((unused)) generator_collect_observable_specs(const ast_component_file_t *component,
                                                                         generator_observable_spec_t *specs,
                                                                         size_t max_specs) {
  size_t index;
  size_t count = 0;

  for (index = 0; index < component->member_count && count < max_specs; ++index) {
    if (component->members[index].is_observable &&
        generator_parse_observable_spec(&component->members[index], &specs[count]) == 0) {
      LOG_TRACE("generator observable member=%s kind=%d path=%s interval=%d seed=%s pipes=%zu\n",
                specs[count].name,
                (int)specs[count].kind,
                specs[count].path,
                specs[count].interval_ms,
                specs[count].seed,
                specs[count].pipe_count);
      count += 1;
    }
  }

  return count;
}

static int generator_is_string_literal(const char *text) {
  size_t length;
  if (text == NULL) {
    return 0;
  }
  length = strlen(text);
  return length >= 2 && text[0] == '\'' && text[length - 1] == '\'';
}

static int generator_is_bool_literal(const char *text) {
  return text != NULL && (strcmp(text, "true") == 0 || strcmp(text, "false") == 0);
}

static int generator_is_numeric_literal(const char *text, int *has_decimal) {
  size_t index = 0;
  int seen_digit = 0;
  int seen_decimal = 0;

  if (has_decimal != NULL) {
    *has_decimal = 0;
  }
  if (text == NULL || text[0] == '\0') {
    return 0;
  }
  if (text[index] == '-' || text[index] == '+') {
    index += 1;
  }
  for (; text[index] != '\0'; ++index) {
    char ch = text[index];
    if (ch >= '0' && ch <= '9') {
      seen_digit = 1;
      continue;
    }
    if (ch == '.' && !seen_decimal) {
      seen_decimal = 1;
      continue;
    }
    return 0;
  }
  if (has_decimal != NULL) {
    *has_decimal = seen_decimal;
  }
  return seen_digit;
}

static generator_field_value_kind_t generator_infer_field_value_kind(const ast_member_t *member) {
  int has_decimal = 0;
  if (member == NULL || member->kind != AST_MEMBER_FIELD || member->initializer[0] == '\0') {
    return GENERATOR_FIELD_VALUE_UNKNOWN;
  }
  if (generator_is_string_literal(member->initializer)) {
    return GENERATOR_FIELD_VALUE_STRING;
  }
  if (generator_is_bool_literal(member->initializer)) {
    return GENERATOR_FIELD_VALUE_BOOL;
  }
  if (generator_is_numeric_literal(member->initializer, &has_decimal)) {
    return has_decimal ? GENERATOR_FIELD_VALUE_DOUBLE : GENERATOR_FIELD_VALUE_INT;
  }
  return GENERATOR_FIELD_VALUE_UNKNOWN;
}

static void generator_strip_string_literal(char *buffer, size_t buffer_size, const char *literal) {
  size_t length;
  if (buffer_size == 0) {
    return;
  }
  if (!generator_is_string_literal(literal)) {
    snprintf(buffer, buffer_size, "%s", literal != NULL ? literal : "");
    return;
  }
  length = strlen(literal);
  if (length <= 2) {
    buffer[0] = '\0';
    return;
  }
  generator_trim_copy(buffer, buffer_size, literal + 1, length - 2);
}

static void generator_default_literal(char *buffer,
                                      size_t buffer_size,
                                      const ast_member_t *member,
                                      generator_field_value_kind_t kind) {
  char string_value[256];

  if (buffer_size == 0) {
    return;
  }

  if (member == NULL || member->initializer[0] == '\0' || kind == GENERATOR_FIELD_VALUE_UNKNOWN) {
    snprintf(buffer, buffer_size, "0");
    return;
  }

  switch (kind) {
    case GENERATOR_FIELD_VALUE_INT:
    case GENERATOR_FIELD_VALUE_DOUBLE:
      snprintf(buffer, buffer_size, "%s", member->initializer);
      return;
    case GENERATOR_FIELD_VALUE_BOOL:
      snprintf(buffer, buffer_size, "%s", strcmp(member->initializer, "true") == 0 ? "1" : "0");
      return;
    case GENERATOR_FIELD_VALUE_STRING:
      generator_strip_string_literal(string_value, sizeof(string_value), member->initializer);
      generator_escape_c_string(buffer, buffer_size, string_value);
      return;
    case GENERATOR_FIELD_VALUE_UNKNOWN:
    default:
      snprintf(buffer, buffer_size, "0");
      return;
  }
}

static void generator_fill_member_spec(const ast_member_t *member, generator_member_spec_t *spec) {
  generator_field_value_kind_t field_kind;
  memset(spec, 0, sizeof(*spec));
  spec->requires_external_fetch = member != NULL ? member->uses_external_fetch : 0;

  if (member == NULL) {
    snprintf(spec->runtime_category, sizeof(spec->runtime_category), "generated-member");
    snprintf(spec->storage_type, sizeof(spec->storage_type), "opaque");
    snprintf(spec->processing_notes, sizeof(spec->processing_notes), "generated without source metadata");
    return;
  }

  if (member->is_observable) {
    snprintf(spec->runtime_category, sizeof(spec->runtime_category), "observable");
    snprintf(spec->storage_type, sizeof(spec->storage_type), "source-defined observable");
    snprintf(spec->processing_notes, sizeof(spec->processing_notes), "derived from observable initializer");
    return;
  }

  if (member->kind == AST_MEMBER_METHOD) {
    snprintf(spec->runtime_category, sizeof(spec->runtime_category), "generated-method");
    snprintf(spec->storage_type, sizeof(spec->storage_type), "callable");
    snprintf(spec->processing_notes, sizeof(spec->processing_notes), "generated from parsed component method");
    return;
  }

  field_kind = generator_infer_field_value_kind(member);
  snprintf(spec->runtime_category, sizeof(spec->runtime_category), "runtime-slot");
  switch (field_kind) {
    case GENERATOR_FIELD_VALUE_INT:
      snprintf(spec->storage_type, sizeof(spec->storage_type), "int");
      break;
    case GENERATOR_FIELD_VALUE_DOUBLE:
      snprintf(spec->storage_type, sizeof(spec->storage_type), "double");
      break;
    case GENERATOR_FIELD_VALUE_BOOL:
      snprintf(spec->storage_type, sizeof(spec->storage_type), "bool");
      break;
    case GENERATOR_FIELD_VALUE_STRING:
      snprintf(spec->storage_type, sizeof(spec->storage_type), "string");
      break;
    case GENERATOR_FIELD_VALUE_UNKNOWN:
    default:
      snprintf(spec->storage_type, sizeof(spec->storage_type), "dynamic");
      break;
  }
  snprintf(spec->processing_notes, sizeof(spec->processing_notes), "generated field accessor derived from initializer");
}

static const char *generator_content_type_for_extension(const char *path) {
  const char *dot = strrchr(path, '.');
  if (dot == NULL) {
    return "text/plain; charset=utf-8";
  }
  if (strcmp(dot, ".json") == 0) {
    return "application/json";
  }
  if (strcmp(dot, ".html") == 0) {
    return "text/html; charset=utf-8";
  }
  if (strcmp(dot, ".css") == 0) {
    return "text/css; charset=utf-8";
  }
  if (strcmp(dot, ".js") == 0) {
    return "application/javascript; charset=utf-8";
  }
  if (strcmp(dot, ".txt") == 0) {
    return "text/plain; charset=utf-8";
  }
  return "application/octet-stream";
}

static void generator_route_path_from_relative(char *buffer,
                                               size_t buffer_size,
                                               const char *relative_path) {
  size_t cursor = 0;
  size_t index = 0;
  size_t last_dot = 0;

  if (buffer_size == 0) {
    return;
  }

  buffer[cursor++] = '/';
  while (relative_path[index] != '\0' && cursor + 2 < buffer_size) {
    char ch = relative_path[index];
    if (ch == '\\') {
      buffer[cursor++] = '/';
    } else {
      buffer[cursor++] = ch;
      if (ch == '.') {
        last_dot = cursor - 1;
      }
    }
    index += 1;
  }
  buffer[cursor] = '\0';

  if (last_dot > 0) {
    buffer[last_dot] = '\0';
  }
}

static int generator_collect_route_file(const char *path, void *context) {
  generator_route_collection_t *collection = (generator_route_collection_t *)context;
  const char *relative_path;
  const char *separator;
  generator_route_asset_t *route;
  size_t prefix_length;

  prefix_length = strlen(collection->routes_root);
  if (_strnicmp(path, collection->routes_root, prefix_length) != 0) {
    return 0;
  }
  relative_path = path + prefix_length;
  if (*relative_path == '\\' || *relative_path == '/') {
    relative_path += 1;
  }
  separator = strchr(relative_path, '\\');
  if (separator == NULL) {
    LOG_TRACE("generator_collect_route_file skip malformed path=%s\n", path);
    return 0;
  }
  if (collection->route_count >= GENERATOR_MAX_ROUTES) {
    log_errorf("too many generated routes under %s\n", collection->routes_root);
    return 1;
  }

  route = &collection->routes[collection->route_count];
  memset(route, 0, sizeof(*route));
  generator_trim_copy(route->method, sizeof(route->method), relative_path, (size_t)(separator - relative_path));
  generator_route_path_from_relative(route->path, sizeof(route->path), separator + 1);
  snprintf(route->content_type, sizeof(route->content_type), "%s", generator_content_type_for_extension(path));

  if (file_read_all(path, &route->body) != 0) {
    log_errorf("failed to read route asset: %s\n", path);
    return 1;
  }

  LOG_TRACE("generator_collect_route_file method=%s path=%s content_type=%s bytes=%zu source=%s\n",
            route->method,
            route->path,
            route->content_type,
            route->body.size,
            path);
  collection->route_count += 1;
  return 0;
}

static int generator_collect_route_assets(const char *input_dir,
                                          generator_route_collection_t *collection) {
  char routes_root[512];

  memset(collection, 0, sizeof(*collection));
  generator_build_path(routes_root, sizeof(routes_root), input_dir, "routes");
  collection->routes_root = _strdup(routes_root);
  if (collection->routes_root == NULL) {
    return 1;
  }

  if (!output_fs_file_exists(routes_root)) {
    LOG_TRACE("generator_collect_route_assets no routes directory path=%s\n", routes_root);
    return 0;
  }

  return path_scan_directory(routes_root, generator_collect_route_file, collection);
}

static void generator_free_route_assets(generator_route_collection_t *collection) {
  size_t index;
  for (index = 0; index < collection->route_count; ++index) {
    file_buffer_free(&collection->routes[index].body);
  }
  if (collection->routes_root != NULL) {
    free((void *)collection->routes_root);
  }
}

static int generator_backend_has_template(const ng_ejs_template_set_t *templates, const char *name) {
  size_t index;
  for (index = 0; index < templates->template_count; ++index) {
    if (strcmp(templates->templates[index].name, name) == 0) {
      return 1;
    }
  }
  return 0;
}

static int generator_collect_backend_assets(const char *input_dir, generator_backend_assets_t *backend) {
  char server_root[512];
  char views_root[512];
  size_t index;

  memset(backend, 0, sizeof(*backend));
  generator_build_path(server_root, sizeof(server_root), input_dir, "server");
  generator_build_path(views_root, sizeof(views_root), server_root, "views");

  if (server_parser_collect(server_root, &backend->routes) != 0) {
    return 1;
  }
  if (ejs_parser_collect(views_root, &backend->templates) != 0) {
    return 1;
  }

  for (index = 0; index < backend->routes.route_count; ++index) {
    const ng_server_route_ir_t *route = &backend->routes.routes[index];
    if (route->response_kind == NG_RESPONSE_RENDER &&
        !generator_backend_has_template(&backend->templates, route->template_name)) {
      log_errorf("missing EJS template for route %s: %s\n", route->path, route->template_name);
      return 1;
    }
  }

  return 0;
}

static void generator_escape_c_string(char *buffer, size_t buffer_size, const char *text) {
  size_t cursor = 0;
  size_t index = 0;

  if (buffer_size == 0) {
    return;
  }

  while (text[index] != '\0' && cursor + 2 < buffer_size) {
    char ch = text[index++];
    if (ch == '\\' || ch == '"') {
      buffer[cursor++] = '\\';
      buffer[cursor++] = ch;
    } else if (ch == '\r') {
      buffer[cursor++] = '\\';
      buffer[cursor++] = 'r';
    } else if (ch == '\n') {
      buffer[cursor++] = '\\';
      buffer[cursor++] = 'n';
    } else {
      buffer[cursor++] = ch;
    }
  }
  buffer[cursor] = '\0';
}

static const ast_member_t *generator_find_member(const ast_component_file_t *component, const char *name) {
  size_t index;
  for (index = 0; index < component->member_count; ++index) {
    if (strcmp(component->members[index].name, name) == 0) {
      return &component->members[index];
    }
  }
  return NULL;
}

static void generator_unquote_literal(char *text) {
  size_t length = strlen(text);
  if (length >= 2 &&
      ((text[0] == '\'' && text[length - 1] == '\'') || (text[0] == '"' && text[length - 1] == '"'))) {
    memmove(text, text + 1, length - 2);
    text[length - 2] = '\0';
  }
}

static int generator_extract_object_property(const char *initializer,
                                             const char *property_name,
                                             char *buffer,
                                             size_t buffer_size) {
  const char *match;
  const char *colon;
  const char *cursor;
  size_t depth = 0;

  if (initializer == NULL || property_name == NULL) {
    return 1;
  }

  match = strstr(initializer, property_name);
  while (match != NULL) {
    if (match == initializer || *(match - 1) == '{' || *(match - 1) == ' ' || *(match - 1) == '\n' || *(match - 1) == ',') {
      break;
    }
    match = strstr(match + 1, property_name);
  }

  if (match == NULL) {
    return 1;
  }

  colon = strchr(match, ':');
  if (colon == NULL) {
    return 1;
  }

  cursor = colon + 1;
  while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') {
    cursor += 1;
  }

  {
    const char *start = cursor;
    while (*cursor != '\0') {
      if (*cursor == '\'' || *cursor == '"') {
        char quote = *cursor++;
        while (*cursor != '\0' && *cursor != quote) {
          if (*cursor == '\\' && *(cursor + 1) != '\0') {
            cursor += 2;
          } else {
            cursor += 1;
          }
        }
        if (*cursor == quote) {
          cursor += 1;
        }
        continue;
      }
      if (*cursor == '{' || *cursor == '[' || *cursor == '(') {
        depth += 1;
      } else if ((*cursor == '}' || *cursor == ']' || *cursor == ')') && depth > 0) {
        depth -= 1;
      } else if ((*(cursor) == ',' || *(cursor) == '}') && depth == 0) {
        break;
      }
      cursor += 1;
    }

    generator_trim_copy(buffer, buffer_size, start, (size_t)(cursor - start));
    generator_unquote_literal(buffer);
    return 0;
  }
}

static int generator_resolve_expression_default(const ast_component_file_t *component,
                                                const char *expression,
                                                char *buffer,
                                                size_t buffer_size);

static int generator_evaluate_equality(const ast_component_file_t *component,
                                       const char *expression,
                                       char *buffer,
                                       size_t buffer_size) {
  const char *eq = strstr(expression, "===");
  char left[128];
  char right[128];
  char left_value[128];
  char right_value[128];

  if (eq == NULL) {
    return 1;
  }

  generator_trim_copy(left, sizeof(left), expression, (size_t)(eq - expression));
  generator_trim_copy(right, sizeof(right), eq + 3, strlen(eq + 3));

  if (generator_resolve_expression_default(component, left, left_value, sizeof(left_value)) != 0) {
    return 1;
  }
  if (generator_resolve_expression_default(component, right, right_value, sizeof(right_value)) != 0) {
    snprintf(right_value, sizeof(right_value), "%s", right);
    generator_unquote_literal(right_value);
  }

  snprintf(buffer, buffer_size, "%s", strcmp(left_value, right_value) == 0 ? "true" : "false");
  return 0;
}

static int generator_resolve_expression_default(const ast_component_file_t *component,
                                                const char *expression,
                                                char *buffer,
                                                size_t buffer_size) {
  char trimmed[256];
  const ast_member_t *member;
  const char *dot;

  generator_trim_copy(trimmed, sizeof(trimmed), expression, strlen(expression));
  if (trimmed[0] == '\0') {
    return 1;
  }

  if (strstr(trimmed, "===") != NULL) {
    return generator_evaluate_equality(component, trimmed, buffer, buffer_size);
  }

  if ((trimmed[0] == '\'' || trimmed[0] == '"')) {
    snprintf(buffer, buffer_size, "%s", trimmed);
    generator_unquote_literal(buffer);
    return 0;
  }

  dot = strchr(trimmed, '.');
  if (dot != NULL) {
    char root[128];
    char property[128];

    generator_trim_copy(root, sizeof(root), trimmed, (size_t)(dot - trimmed));
    generator_trim_copy(property, sizeof(property), dot + 1, strlen(dot + 1));
    member = generator_find_member(component, root);
    if (member != NULL && member->initializer[0] != '\0') {
      return generator_extract_object_property(member->initializer, property, buffer, buffer_size);
    }
    return 1;
  }

  member = generator_find_member(component, trimmed);
  if (member != NULL && member->initializer[0] != '\0') {
    snprintf(buffer, buffer_size, "%s", member->initializer);
    generator_trim_copy(buffer, buffer_size, buffer, strlen(buffer));
    generator_unquote_literal(buffer);
    return 0;
  }

  if (strcmp(trimmed, "true") == 0 || strcmp(trimmed, "false") == 0) {
    snprintf(buffer, buffer_size, "%s", trimmed);
    return 0;
  }

  return 1;
}

static int generator_compile_template_html(const ast_component_file_t *component,
                                           const char *html_source,
                                           char *buffer,
                                           size_t buffer_size) {
  size_t index = 0;
  size_t cursor = 0;
  size_t interpolation_count = 0;
  size_t attr_count = 0;
  size_t class_count = 0;
  size_t click_count = 0;

  while (html_source[index] != '\0' && cursor + 1 < buffer_size) {
    if (html_source[index] == '{' && html_source[index + 1] == '{') {
      size_t expr_start = index + 2;
      size_t expr_end = expr_start;
      char expr[256];
      char value[256];

      while (html_source[expr_end] != '\0' &&
             !(html_source[expr_end] == '}' && html_source[expr_end + 1] == '}')) {
        expr_end += 1;
      }
      generator_trim_copy(expr, sizeof(expr), html_source + expr_start, expr_end - expr_start);
      if (generator_resolve_expression_default(component, expr, value, sizeof(value)) != 0) {
        value[0] = '\0';
      }
      LOG_TRACE("template interpolation expr=%s value=%s\n", expr, value);
      interpolation_count += 1;
      cursor += (size_t)snprintf(buffer + cursor,
                                 buffer_size - cursor,
                                 "<span data-ng-text=\"%s\">%s</span>",
                                 expr,
                                 value);
      index = html_source[expr_end] != '\0' ? expr_end + 2 : expr_end;
      continue;
    }

    if (html_source[index] == '[') {
      if (strncmp(html_source + index, "[attr.", 6) == 0) {
        size_t name_start = index + 6;
        size_t name_end = name_start;
        const char *equals_sign;
        const char *quote_start;
        const char *quote_end;
        char attr_name[64];
        char expr[256];
        char value[256];

        while (html_source[name_end] != '\0' && html_source[name_end] != ']') {
          name_end += 1;
        }
        equals_sign = strchr(html_source + name_end, '=');
        quote_start = equals_sign != NULL ? strchr(equals_sign, '"') : NULL;
        quote_end = quote_start != NULL ? strchr(quote_start + 1, '"') : NULL;
        if (html_source[name_end] == '\0' ||
            equals_sign == NULL ||
            quote_start == NULL ||
            quote_end == NULL) {
          LOG_TRACE("template attr binding malformed near=%.32s\n", html_source + index);
          buffer[cursor++] = html_source[index++];
          continue;
        }
        generator_trim_copy(attr_name, sizeof(attr_name), html_source + name_start, name_end - name_start);
        generator_trim_copy(expr, sizeof(expr), quote_start + 1, (size_t)(quote_end - (quote_start + 1)));
        if (generator_resolve_expression_default(component, expr, value, sizeof(value)) != 0) {
          value[0] = '\0';
        }
        LOG_TRACE("template attr binding attr=%s expr=%s value=%s\n", attr_name, expr, value);
        attr_count += 1;
        cursor += (size_t)snprintf(buffer + cursor,
                                   buffer_size - cursor,
                                   "%s=\"%s\" data-ng-attr-%s=\"%s\"",
                                   attr_name,
                                   value,
                                   attr_name,
                                   expr);
        index = (size_t)(quote_end - html_source) + 1;
        continue;
      }

      if (strncmp(html_source + index, "[class.", 7) == 0) {
        size_t name_start = index + 7;
        size_t name_end = name_start;
        const char *equals_sign;
        const char *quote_start;
        const char *quote_end;
        char class_name[64];
        char expr[256];
        char value[64];

        while (html_source[name_end] != '\0' && html_source[name_end] != ']') {
          name_end += 1;
        }
        equals_sign = strchr(html_source + name_end, '=');
        quote_start = equals_sign != NULL ? strchr(equals_sign, '"') : NULL;
        quote_end = quote_start != NULL ? strchr(quote_start + 1, '"') : NULL;
        if (html_source[name_end] == '\0' ||
            equals_sign == NULL ||
            quote_start == NULL ||
            quote_end == NULL) {
          LOG_TRACE("template class binding malformed near=%.32s\n", html_source + index);
          buffer[cursor++] = html_source[index++];
          continue;
        }
        generator_trim_copy(class_name, sizeof(class_name), html_source + name_start, name_end - name_start);
        generator_trim_copy(expr, sizeof(expr), quote_start + 1, (size_t)(quote_end - (quote_start + 1)));
        if (generator_resolve_expression_default(component, expr, value, sizeof(value)) != 0) {
          snprintf(value, sizeof(value), "false");
        }
        LOG_TRACE("template class binding class=%s expr=%s value=%s\n", class_name, expr, value);
        class_count += 1;
        cursor += (size_t)snprintf(buffer + cursor,
                                   buffer_size - cursor,
                                   "data-ng-class-%s=\"%s\"",
                                   class_name,
                                   expr);
        index = (size_t)(quote_end - html_source) + 1;
        continue;
      }
    }

    if (html_source[index] == '(' && strncmp(html_source + index, "(click)", 7) == 0) {
      size_t expr_start = index + 9;
      size_t expr_end = expr_start;
      char expr[256];

      while (html_source[expr_end] != '\0' && html_source[expr_end] != '"') {
        expr_end += 1;
      }
      generator_trim_copy(expr, sizeof(expr), html_source + expr_start, expr_end - expr_start);
      LOG_TRACE("template click binding expr=%s\n", expr);
      click_count += 1;
      cursor += (size_t)snprintf(buffer + cursor,
                                 buffer_size - cursor,
                                 "data-ng-click=\"%s\"",
                                 expr);
      index = expr_end + 1;
      continue;
    }

    buffer[cursor++] = html_source[index++];
  }

  buffer[cursor] = '\0';
  LOG_TRACE("template compile done interpolations=%zu attrs=%zu classes=%zu clicks=%zu output_bytes=%zu\n",
            interpolation_count,
            attr_count,
            class_count,
            click_count,
            cursor);
  return 0;
}

static void generator_fill_member_prototype(const ast_component_file_t *component,
                                            const ast_member_t *member,
                                            char *buffer,
                                            size_t buffer_size) {
  js_codegen_result_t codegen;
  char safe_name[128];
  generator_field_value_kind_t field_kind;
  generator_make_safe_name(safe_name, sizeof(safe_name), member->name);

  if (member->kind == AST_MEMBER_FIELD) {
    field_kind = generator_infer_field_value_kind(member);
    switch (field_kind) {
      case GENERATOR_FIELD_VALUE_INT:
        snprintf(buffer, buffer_size, "int angular_%s_get(const ng_runtime_t *runtime);", safe_name);
        return;
      case GENERATOR_FIELD_VALUE_DOUBLE:
        snprintf(buffer, buffer_size, "double angular_%s_get(const ng_runtime_t *runtime);", safe_name);
        return;
      case GENERATOR_FIELD_VALUE_BOOL:
        snprintf(buffer, buffer_size, "bool angular_%s_get(const ng_runtime_t *runtime);", safe_name);
        return;
      case GENERATOR_FIELD_VALUE_STRING:
      case GENERATOR_FIELD_VALUE_UNKNOWN:
      default:
        snprintf(buffer, buffer_size, "const char *angular_%s_get(const ng_runtime_t *runtime);", safe_name);
        return;
    }
  }

  if (member->uses_external_fetch) {
    snprintf(buffer,
             buffer_size,
             "int angular_%s_call(ng_runtime_t *runtime, ng_fetch_text_fn fetch_fn, void *fetch_context);",
             safe_name);
    return;
  }

  if (member->kind == AST_MEMBER_METHOD && js_codegen_compile_method(component, member, &codegen) == 0 && codegen.supported) {
    snprintf(buffer, buffer_size, "%s", codegen.prototype);
    return;
  }

  snprintf(buffer, buffer_size, "void angular_%s_generated_stub(void);", safe_name);
}

static void generator_fill_member_definition(const ast_component_file_t *component,
                                             const ast_member_t *member,
                                             char *buffer,
                                             size_t buffer_size) {
  char safe_name[128];
  generator_member_spec_t spec;
  generator_field_value_kind_t field_kind;
  char default_literal[512];
  generator_make_safe_name(safe_name, sizeof(safe_name), member->name);
  generator_fill_member_spec(member, &spec);

  if (member->kind == AST_MEMBER_FIELD) {
    field_kind = generator_infer_field_value_kind(member);
    generator_default_literal(default_literal, sizeof(default_literal), member, field_kind);

    switch (field_kind) {
      case GENERATOR_FIELD_VALUE_INT:
        snprintf(buffer,
                 buffer_size,
                 "const angular_%s_header_t angular_%s_header = {\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  \"field\",\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  %d\n"
                 "};\n\n"
                 "int angular_%s_get(const ng_runtime_t *runtime) {\n"
                 "  return ng_runtime_get_int(runtime, \"%s\", %s);\n"
                 "}\n",
                 safe_name,
                 safe_name,
                 component->class_name,
                 member->name,
                 spec.runtime_category,
                 spec.storage_type,
                 spec.processing_notes,
                 spec.requires_external_fetch,
                 safe_name,
                 member->name,
                 default_literal);
        return;
      case GENERATOR_FIELD_VALUE_DOUBLE:
        snprintf(buffer,
                 buffer_size,
                 "const angular_%s_header_t angular_%s_header = {\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  \"field\",\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  %d\n"
                 "};\n\n"
                 "double angular_%s_get(const ng_runtime_t *runtime) {\n"
                 "  return ng_runtime_get_double(runtime, \"%s\", %s);\n"
                 "}\n",
                 safe_name,
                 safe_name,
                 component->class_name,
                 member->name,
                 spec.runtime_category,
                 spec.storage_type,
                 spec.processing_notes,
                 spec.requires_external_fetch,
                 safe_name,
                 member->name,
                 default_literal);
        return;
      case GENERATOR_FIELD_VALUE_BOOL:
        snprintf(buffer,
                 buffer_size,
                 "const angular_%s_header_t angular_%s_header = {\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  \"field\",\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  %d\n"
                 "};\n\n"
                 "bool angular_%s_get(const ng_runtime_t *runtime) {\n"
                 "  return ng_runtime_get_bool(runtime, \"%s\", %s) != 0;\n"
                 "}\n",
                 safe_name,
                 safe_name,
                 component->class_name,
                 member->name,
                 spec.runtime_category,
                 spec.storage_type,
                 spec.processing_notes,
                 spec.requires_external_fetch,
                 safe_name,
                 member->name,
                 default_literal);
        return;
      case GENERATOR_FIELD_VALUE_STRING:
        snprintf(buffer,
                 buffer_size,
                 "const angular_%s_header_t angular_%s_header = {\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  \"field\",\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  %d\n"
                 "};\n\n"
                 "const char *angular_%s_get(const ng_runtime_t *runtime) {\n"
                 "  return ng_runtime_get_string(runtime, \"%s\", \"%s\");\n"
                 "}\n",
                 safe_name,
                 safe_name,
                 component->class_name,
                 member->name,
                 spec.runtime_category,
                 spec.storage_type,
                 spec.processing_notes,
                 spec.requires_external_fetch,
                 safe_name,
                 member->name,
                 default_literal);
        return;
      case GENERATOR_FIELD_VALUE_UNKNOWN:
      default:
        snprintf(buffer,
                 buffer_size,
                 "const angular_%s_header_t angular_%s_header = {\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  \"field\",\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  \"%s\",\n"
                 "  %d\n"
                 "};\n\n"
                 "const char *angular_%s_get(const ng_runtime_t *runtime) {\n"
                 "  return ng_runtime_get_string(runtime, \"%s\", \"\");\n"
                 "}\n",
                 safe_name,
                 safe_name,
                 component->class_name,
                 member->name,
                 spec.runtime_category,
                 spec.storage_type,
                 spec.processing_notes,
                 spec.requires_external_fetch,
                 safe_name,
                 member->name);
        return;
    }
  }

  if (member->kind == AST_MEMBER_METHOD) {
    js_codegen_result_t codegen;
    if (js_codegen_compile_method(component, member, &codegen) == 0 && codegen.supported) {
      snprintf(buffer,
               buffer_size,
               "const angular_%s_header_t angular_%s_header = {\n"
               "  \"%s\",\n"
               "  \"%s\",\n"
               "  \"method\",\n"
               "  \"%s\",\n"
               "  \"%s\",\n"
               "  \"%s\",\n"
               "  %d\n"
               "};\n\n"
               "%s",
               safe_name,
               safe_name,
               component->class_name,
               member->name,
               spec.runtime_category,
               js_type_name(codegen.return_type),
               spec.processing_notes,
               spec.requires_external_fetch,
               codegen.definition);
      return;
    }

    if (member->uses_external_fetch) {
      snprintf(buffer,
               buffer_size,
               "const angular_%s_header_t angular_%s_header = {\n"
               "  \"%s\",\n"
               "  \"%s\",\n"
               "  \"method\",\n"
               "  \"%s\",\n"
               "  \"%s\",\n"
               "  \"%s\",\n"
               "  %d\n"
               "};\n\n"
               "int angular_%s_call(ng_runtime_t *runtime, ng_fetch_text_fn fetch_fn, void *fetch_context) {\n"
               "  (void)runtime;\n"
               "  (void)fetch_fn;\n"
               "  (void)fetch_context;\n"
               "  return 0;\n"
               "}\n",
               safe_name,
               safe_name,
               component->class_name,
               member->name,
               spec.runtime_category,
               spec.storage_type,
               "declared as external fetch effect; destination project supplies the implementation contract",
               1,
               safe_name);
      return;
    }
  }

  snprintf(buffer,
           buffer_size,
           "const angular_%s_header_t angular_%s_header = {\n"
           "  \"%s\",\n"
           "  \"%s\",\n"
           "  \"%s\",\n"
           "  \"%s\",\n"
           "  \"%s\",\n"
           "  \"%s\",\n"
           "  %d\n"
           "};\n\n"
           "void angular_%s_generated_stub(void) {\n"
           "}\n",
           safe_name,
           safe_name,
           component->class_name,
           member->name,
           member->kind == AST_MEMBER_METHOD ? "method" : "field",
           spec.runtime_category,
           spec.storage_type,
           spec.processing_notes,
           spec.requires_external_fetch,
           safe_name);
}

static int generator_emit_member_source(const char *output_dir,
                                        const ast_component_file_t *component,
                                        const ast_member_t *member) {
  char filename[256];
  char path[512];
  char definition[4096];
  char body[5120];
  char extra_includes[512];
  char safe_name[128];
  size_t index;

  extra_includes[0] = '\0';
  generator_make_safe_name(safe_name, sizeof(safe_name), member->name);

  snprintf(filename, sizeof(filename), "angular_%s.c", safe_name);
  generator_build_path(path, sizeof(path), output_dir, filename);
  generator_fill_member_definition(component, member, definition, sizeof(definition));
  for (index = 0; index < component->member_count; ++index) {
    char needle[160];
    char dependency_safe_name[128];
    if (component->members[index].kind != AST_MEMBER_METHOD ||
        strcmp(component->members[index].name, member->name) == 0) {
      continue;
    }
    snprintf(needle, sizeof(needle), "this.%s(", component->members[index].name);
    if (strstr(member->body, needle) == NULL) {
      continue;
    }
    generator_make_safe_name(dependency_safe_name, sizeof(dependency_safe_name), component->members[index].name);
    snprintf(extra_includes + strlen(extra_includes),
             sizeof(extra_includes) - strlen(extra_includes),
             "#include \"angular_%s.h\"\n",
             dependency_safe_name);
  }
  snprintf(body,
           sizeof(body),
           "#include \"angular_%s.h\"\n"
           "%s"
           "#include \"helpers/include/math/number_utils.h\"\n\n"
           "%s",
           safe_name,
           extra_includes,
           definition);
  return output_fs_write_text(path, body);
}

static int generator_emit_http_service_header(const char *output_dir,
                                              size_t static_route_count,
                                              size_t total_route_count) {
  char header_text[4096];

  snprintf(header_text,
           sizeof(header_text),
           "#ifndef ANGULAR_HTTP_SERVICE_H\n"
           "#define ANGULAR_HTTP_SERVICE_H\n\n"
           "#include \"helpers/include/runtime/app_runtime.h\"\n"
           "#include \"helpers/include/net/http_service.h\"\n\n"
           "#define ANGULAR_STATIC_ROUTE_COUNT %zu\n"
           "#define ANGULAR_GENERATED_ROUTE_COUNT %zu\n\n"
           "typedef struct {\n"
           "  const char *method;\n"
           "  const char *path;\n"
           "  const char *content_type;\n"
           "  const char *body;\n"
           "} angular_generated_route_t;\n\n"
           "typedef struct {\n"
           "  ng_runtime_t *runtime;\n"
           "  ng_http_service_t service;\n"
           "  ng_http_route_t routes[ANGULAR_GENERATED_ROUTE_COUNT > 0 ? ANGULAR_GENERATED_ROUTE_COUNT : 1];\n"
           "  angular_generated_route_t generated_routes[ANGULAR_STATIC_ROUTE_COUNT > 0 ? ANGULAR_STATIC_ROUTE_COUNT : 1];\n"
           "} angular_http_service_t;\n\n"
           "void angular_http_service_init(angular_http_service_t *service,\n"
           "                               ng_runtime_t *runtime,\n"
           "                               const char *html_page,\n"
           "                               const char *css_text,\n"
           "                               const char *js_text);\n\n"
           "#endif\n",
           static_route_count,
           total_route_count);

  return generator_write_text_asset(output_dir, "angular_http_service.h", header_text);
}

static int generator_emit_http_service_source(const char *output_dir,
                                              const generator_route_collection_t *routes,
                                              const generator_backend_assets_t *backend) {
  char *source_text;
  char *route_bodies;
  char *route_table;
  char *route_init;
  ng_server_codegen_result_t *backend_codegen;
  int result;
  size_t body_cursor = 0;
  size_t table_cursor = 0;
  size_t init_cursor = 0;
  size_t index;

  source_text = (char *)calloc(1, 524288);
  route_bodies = (char *)calloc(1, 262144);
  route_table = (char *)calloc(1, 16384);
  route_init = (char *)calloc(1, 16384);
  backend_codegen = (ng_server_codegen_result_t *)calloc(1, sizeof(*backend_codegen));
  if (source_text == NULL || route_bodies == NULL || route_table == NULL || route_init == NULL || backend_codegen == NULL) {
    free(source_text);
    free(route_bodies);
    free(route_table);
    free(route_init);
    free(backend_codegen);
    return 1;
  }

  if (server_codegen_emit(&backend->routes, &backend->templates, routes->route_count, backend_codegen) != 0) {
    free(source_text);
    free(route_bodies);
    free(route_table);
    free(route_init);
    free(backend_codegen);
    return 1;
  }

  for (index = 0; index < routes->route_count; ++index) {
    char escaped_body[65536];
    const generator_route_asset_t *route = &routes->routes[index];
    generator_escape_c_string(escaped_body, sizeof(escaped_body), route->body.data);
    LOG_TRACE("generator_emit_http_service_source route=%zu path=%s raw_bytes=%zu escaped_chars=%zu\n",
              index,
              route->path,
              route->body.size,
              strlen(escaped_body));
    body_cursor += (size_t)snprintf(route_bodies + body_cursor,
                                    262144 - body_cursor,
                                    "static const char g_route_body_%zu[] = \"%s\";\n",
                                    index,
                                    escaped_body);
    table_cursor += (size_t)snprintf(route_table + table_cursor,
                                     16384 - table_cursor,
                                     "  { \"%s\", \"%s\", \"%s\", g_route_body_%zu },\n",
                                     route->method,
                                     route->path,
                                     route->content_type,
                                     index);
    init_cursor += (size_t)snprintf(route_init + init_cursor,
                                    16384 - init_cursor,
                                    "  service->routes[%zu].method = service->generated_routes[%zu].method;\n"
                                    "  service->routes[%zu].path = service->generated_routes[%zu].path;\n"
                                    "  service->routes[%zu].handler = angular_http_write_generated_route;\n"
                                    "  service->routes[%zu].context = &service->generated_routes[%zu];\n",
                                    index, index,
                                    index, index,
                                    index,
                                    index, index);
  }

  snprintf(source_text,
           524288,
           "#include \"angular_http_service.h\"\n\n"
           "#include <stdio.h>\n"
           "#include <string.h>\n"
           "#include <stdlib.h>\n"
           "#include \"helpers/include/data/json.h\"\n\n"
           "#include \"helpers/include/support/stringbuilder.h\"\n\n"
           "#include \"helpers/include/runtime/server_runtime.h\"\n\n"
           "%s\n"
           "%s\n"
           "static const angular_generated_route_t g_generated_routes[ANGULAR_STATIC_ROUTE_COUNT > 0 ? ANGULAR_STATIC_ROUTE_COUNT : 1] = {\n"
           "%s"
           "};\n\n"
           "static void angular_http_log(const char *message) {\n"
           "  fprintf(stdout, \"[angular_http] %%s\\n\", message);\n"
           "  fflush(stdout);\n"
           "}\n\n"
           "static void angular_http_write_error_json(ng_http_response_t *response, const char *message) {\n"
           "  json_data *root = init_json_object();\n"
           "  char *json_text = NULL;\n"
           "  if (root != NULL) {\n"
           "    json_object_add_string(root, \"error\", message != NULL ? message : \"unknown error\");\n"
           "    json_text = json_tostring(root);\n"
           "    json_free(root);\n"
           "  }\n"
           "  ng_http_response_set_text(response, json_text != NULL ? json_text : \"{}\");\n"
           "  free(json_text);\n"
           "}\n\n"
           "static int angular_http_write_generated_route(void *context,\n"
           "                                             const ng_http_request_t *request,\n"
           "                                             ng_http_response_t *response) {\n"
           "  const angular_generated_route_t *route = (const angular_generated_route_t *)context;\n"
           "  (void)request;\n"
           "  if (route == NULL) {\n"
           "    response->status_code = 500;\n"
           "    angular_http_write_error_json(response, \"missing generated route context\");\n"
           "    return 0;\n"
           "  }\n"
           "  angular_http_log(route->path);\n"
           "  snprintf(response->content_type, sizeof(response->content_type), \"%%s\", route->content_type);\n"
           "  if (ng_http_response_set_text(response, route->body) != 0) {\n"
           "    response->status_code = 500;\n"
           "    angular_http_write_error_json(response, \"route body too large\");\n"
           "  }\n"
           "  return 0;\n"
           "}\n\n"
           "%s\n"
           "void angular_http_service_init(angular_http_service_t *service,\n"
           "                               ng_runtime_t *runtime,\n"
           "                               const char *html_page,\n"
           "                               const char *css_text,\n"
           "                               const char *js_text) {\n"
           "  size_t index;\n"
           "  service->runtime = runtime;\n"
           "  for (index = 0; index < ANGULAR_STATIC_ROUTE_COUNT; ++index) {\n"
           "    service->generated_routes[index] = g_generated_routes[index];\n"
           "  }\n"
           "%s"
           "%s"
           "  ng_http_service_init(&service->service, html_page, css_text, js_text, service->routes, ANGULAR_GENERATED_ROUTE_COUNT);\n"
           "}\n",
           route_bodies,
           backend_codegen->support_source,
           route_table,
           backend_codegen->route_source,
           route_init,
           backend_codegen->route_init);

  result = generator_write_text_asset(output_dir, "angular_http_service.c", source_text);
  free(source_text);
  free(route_bodies);
  free(route_table);
  free(route_init);
  free(backend_codegen);
  return result;
}

static int generator_emit_demo_file(const char *output_dir) {
  const char *demo_source =
      "#include <stdlib.h>\n"
      "#include <stdio.h>\n"
      "#include <string.h>\n"
      "#include \"angular_http_service.h\"\n"
      "#include \"helpers/include/net/http_server.h\"\n"
      "#include \"helpers/include/net/http_service.h\"\n\n"
      "static char *g_index_html = NULL;\n"
      "static char *g_styles_css = NULL;\n"
      "static char *g_app_js = NULL;\n\n"
      "static int angular_demo_load_file(const char *path, char **buffer, size_t *buffer_size) {\n"
      "  FILE *file = fopen(path, \"rb\");\n"
      "  long file_size;\n"
      "  size_t bytes_read;\n"
      "  if (file == NULL) {\n"
      "    fprintf(stderr, \"[demo] failed to open %s\\n\", path);\n"
      "    return 1;\n"
      "  }\n"
      "  if (fseek(file, 0, SEEK_END) != 0) {\n"
      "    fclose(file);\n"
      "    fprintf(stderr, \"[demo] failed to seek %s\\n\", path);\n"
      "    return 1;\n"
      "  }\n"
      "  file_size = ftell(file);\n"
      "  if (file_size < 0) {\n"
      "    fclose(file);\n"
      "    fprintf(stderr, \"[demo] failed to size %s\\n\", path);\n"
      "    return 1;\n"
      "  }\n"
      "  if (fseek(file, 0, SEEK_SET) != 0) {\n"
      "    fclose(file);\n"
      "    fprintf(stderr, \"[demo] failed to rewind %s\\n\", path);\n"
      "    return 1;\n"
      "  }\n"
      "  *buffer = (char *)malloc((size_t)file_size + 1u);\n"
      "  if (*buffer == NULL) {\n"
      "    fclose(file);\n"
      "    fprintf(stderr, \"[demo] failed to allocate %s bytes=%ld\\n\", path, file_size);\n"
      "    return 1;\n"
      "  }\n"
      "  bytes_read = fread(*buffer, 1, (size_t)file_size, file);\n"
      "  if (ferror(file)) {\n"
      "    free(*buffer);\n"
      "    *buffer = NULL;\n"
      "    fclose(file);\n"
      "    fprintf(stderr, \"[demo] failed to read %s\\n\", path);\n"
      "    return 1;\n"
      "  }\n"
      "  if (bytes_read != (size_t)file_size) {\n"
      "    free(*buffer);\n"
      "    *buffer = NULL;\n"
      "    fclose(file);\n"
      "    fprintf(stderr, \"[demo] short read %s expected=%ld actual=%zu\\n\", path, file_size, bytes_read);\n"
      "    return 1;\n"
      "  }\n"
      "  (*buffer)[bytes_read] = '\\0';\n"
      "  *buffer_size = bytes_read;\n"
      "  fclose(file);\n"
      "  printf(\"[demo] loaded %s bytes=%zu\\n\", path, bytes_read);\n"
      "  fflush(stdout);\n"
      "  return 0;\n"
      "}\n\n"
      "int main(int argc, char **argv) {\n"
      "  ng_runtime_t runtime;\n"
      "  angular_http_service_t service;\n"
      "  unsigned short port = 18080;\n"
      "  int max_requests = 0;\n\n"
      "  if (argc >= 2) {\n"
      "    port = (unsigned short)atoi(argv[1]);\n"
      "  }\n"
      "  if (argc >= 3) {\n"
      "    max_requests = atoi(argv[2]);\n"
      "  }\n\n"
      "  size_t index_html_size = 0;\n"
      "  size_t styles_css_size = 0;\n"
      "  size_t app_js_size = 0;\n\n"
      "  if (angular_demo_load_file(\"index.html\", &g_index_html, &index_html_size) != 0 ||\n"
      "      angular_demo_load_file(\"styles.css\", &g_styles_css, &styles_css_size) != 0 ||\n"
      "      angular_demo_load_file(\"app.js\", &g_app_js, &app_js_size) != 0) {\n"
      "    free(g_index_html);\n"
      "    free(g_styles_css);\n"
      "    free(g_app_js);\n"
      "    return 1;\n"
      "  }\n\n"
      "  ng_runtime_init(&runtime);\n"
      "  (void)index_html_size;\n"
      "  (void)styles_css_size;\n"
      "  (void)app_js_size;\n"
      "  printf(\"[demo] starting port=%u max_requests=%d\\n\", port, max_requests);\n"
      "  fflush(stdout);\n"
      "  angular_http_service_init(&service, &runtime, g_index_html, g_styles_css, g_app_js);\n"
      "  max_requests = ng_http_server_serve(port, ng_http_service_handle, &service.service, max_requests);\n"
      "  free(g_index_html);\n"
      "  free(g_styles_css);\n"
      "  free(g_app_js);\n"
      "  return max_requests;\n"
      "}\n";

  return generator_write_text_asset(output_dir, "angular_generated_demo.c", demo_source);
}

static int generator_emit_index_html(const char *output_dir,
                                     const ast_component_file_t *component,
                                     const char *html_source) {
  char compiled_html[65536];
  char document_html[98304];
  LOG_TRACE("generator_emit_index_html start html_bytes=%zu class=%s\n",
            html_source != NULL ? strlen(html_source) : 0u,
            component != NULL ? component->class_name : "<null>");
  if (generator_compile_template_html(component, html_source, compiled_html, sizeof(compiled_html)) != 0) {
    return 1;
  }
  LOG_TRACE("generator_emit_index_html preview=%.*s\n", 180, compiled_html);
  if (strstr(compiled_html, "<!DOCTYPE html>") != NULL || strstr(compiled_html, "<html") != NULL) {
    LOG_TRACE("generator_emit_index_html detected full document input\n");
    return generator_write_text_asset(output_dir, "index.html", compiled_html);
  }
  snprintf(document_html,
           sizeof(document_html),
           "<!DOCTYPE html>\n"
           "<html lang=\"en\">\n"
           "<head>\n"
           "  <meta charset=\"utf-8\">\n"
           "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
           "  <title>Angular Generated App</title>\n"
           "  <link rel=\"stylesheet\" href=\"/styles.css\">\n"
           "</head>\n"
           "<body>\n"
           "%s\n"
           "  <script src=\"/app.js\"></script>\n"
           "</body>\n"
           "</html>\n",
           compiled_html);
  LOG_TRACE("generator_emit_index_html document_preview=%.*s\n", 220, document_html);
  return generator_write_text_asset(output_dir, "index.html", document_html);
}

static int generator_emit_styles_css(const char *output_dir, const char *css_source) {
  return generator_write_text_asset(output_dir, "styles.css", css_source);
}

static const char *generator_pipe_kind_name(generator_pipe_kind_t kind) {
  switch (kind) {
    case GENERATOR_PIPE_PROP:
      return "prop";
    case GENERATOR_PIPE_MAP:
      return "map";
    case GENERATOR_PIPE_TAP:
      return "tap";
    case GENERATOR_PIPE_REDUCE:
      return "reduce";
    case GENERATOR_PIPE_EFFECT_CLASS:
      return "effect-class";
    case GENERATOR_PIPE_EFFECT_CLASS_AT:
      return "effect-class-at";
    case GENERATOR_PIPE_EFFECT_STAGGER_CLASS:
      return "effect-stagger-class";
    case GENERATOR_PIPE_EFFECT_STYLE_VAR:
      return "effect-style-var";
    case GENERATOR_PIPE_UNKNOWN:
    default:
      return "unknown";
  }
}

static int generator_append_text(char *buffer, size_t buffer_size, size_t *cursor, const char *text) {
  int written;
  if (*cursor >= buffer_size) {
    return 1;
  }
  written = snprintf(buffer + *cursor, buffer_size - *cursor, "%s", text);
  if (written < 0 || (size_t)written >= buffer_size - *cursor) {
    return 1;
  }
  *cursor += (size_t)written;
  return 0;
}

static int generator_append_format(char *buffer, size_t buffer_size, size_t *cursor, const char *format, ...) {
  va_list args;
  int written;
  if (*cursor >= buffer_size) {
    return 1;
  }
  va_start(args, format);
  written = vsnprintf(buffer + *cursor, buffer_size - *cursor, format, args);
  va_end(args);
  if (written < 0 || (size_t)written >= buffer_size - *cursor) {
    return 1;
  }
  *cursor += (size_t)written;
  return 0;
}

static int generator_append_runtime_asset(char *buffer, size_t buffer_size, size_t *cursor) {
  file_buffer_t runtime_asset;
  const char *asset_path = "assets\\ng_runtime.js";

  memset(&runtime_asset, 0, sizeof(runtime_asset));
  LOG_TRACE("generator_append_runtime_asset path=%s\n", asset_path);
  if (file_read_all(asset_path, &runtime_asset) != 0) {
    log_errorf("failed to read runtime asset: %s\n", asset_path);
    return 1;
  }

  if (generator_append_text(buffer, buffer_size, cursor, runtime_asset.data) != 0) {
    file_buffer_free(&runtime_asset);
    log_errorf("failed to append runtime asset: %s\n", asset_path);
    return 1;
  }

  file_buffer_free(&runtime_asset);
  return 0;
}

static int generator_emit_compiled_app_js(const char *output_dir,
                                          const char *input_dir,
                                          const ast_component_file_t *component) {
  generator_observable_spec_t specs[GENERATOR_MAX_OBSERVABLES];
  size_t spec_count;
  size_t index;
  file_buffer_t source_app_js;
  char app_js[262144];
  size_t cursor = 0;
  char source_path[512];

  generator_build_path(source_path, sizeof(source_path), input_dir, "app.js");
  if (file_read_all(source_path, &source_app_js) != 0) {
    log_errorf("failed to read source app.js: %s\n", source_path);
    return 1;
  }

  spec_count = generator_collect_observable_specs(component, specs, GENERATOR_MAX_OBSERVABLES);
  LOG_TRACE("generator_emit_compiled_app_js spec_count=%zu source=%s\n", spec_count, source_path);

  if (generator_append_runtime_asset(app_js, sizeof(app_js), &cursor) != 0) {
    file_buffer_free(&source_app_js);
    return 1;
  }

  if (generator_append_text(app_js, sizeof(app_js), &cursor, source_app_js.data) != 0) {
    file_buffer_free(&source_app_js);
    return 1;
  }

  if (generator_append_text(app_js, sizeof(app_js), &cursor,
                            "\n\n  var ngApp = ngCreateApp();\n"
                            "  window.ngApp = ngApp;\n") != 0) {
    file_buffer_free(&source_app_js);
    return 1;
  }

  for (index = 0; index < spec_count; ++index) {
    size_t step_index;
    size_t argument_index;
    char alias[128];
    char steps_text[2048];
    size_t steps_cursor = 0;

    steps_text[0] = '\0';

    generator_strip_trailing_dollar(alias, sizeof(alias), specs[index].name);
    if (specs[index].kind == GENERATOR_OBSERVABLE_STATE) {
      if (generator_append_format(app_js,
                                  sizeof(app_js),
                                  &cursor,
                                  "  ngApp.registerObservable({ name: '%s', alias: '%s', kind: 'state', path: '', seed: '%s', intervalMs: %d, steps: [",
                                  specs[index].name,
                                  alias,
                                  specs[index].seed,
                                  specs[index].interval_ms) != 0) {
        file_buffer_free(&source_app_js);
        return 1;
      }
    } else {
      if (generator_append_format(app_js,
                                  sizeof(app_js),
                                  &cursor,
                                  "  ngApp.registerObservable({ name: '%s', alias: '%s', kind: '%s', path: '%s', seed: null, intervalMs: %d, steps: [",
                                  specs[index].name,
                                  alias,
                                  specs[index].kind == GENERATOR_OBSERVABLE_POLL ? "poll" :
                                      specs[index].kind == GENERATOR_OBSERVABLE_POST ? "post" : "unknown",
                                  specs[index].path,
                                  specs[index].interval_ms) != 0) {
        file_buffer_free(&source_app_js);
        return 1;
      }
    }
    for (step_index = 0; step_index < specs[index].pipe_count; ++step_index) {
      if (step_index > 0) {
        if (generator_append_text(steps_text, sizeof(steps_text), &steps_cursor, ", ") != 0) {
          file_buffer_free(&source_app_js);
          return 1;
        }
      }
      if (generator_append_format(steps_text,
                                  sizeof(steps_text),
                                  &steps_cursor,
                                  "{ kind: '%s', argument: '%s', args: [",
                                  generator_pipe_kind_name(specs[index].pipe_ops[step_index].kind),
                                  specs[index].pipe_ops[step_index].argument) != 0) {
        file_buffer_free(&source_app_js);
        return 1;
      }
      for (argument_index = 0; argument_index < specs[index].pipe_ops[step_index].argument_count; ++argument_index) {
        if (argument_index > 0) {
          if (generator_append_text(steps_text, sizeof(steps_text), &steps_cursor, ", ") != 0) {
            file_buffer_free(&source_app_js);
            return 1;
          }
        }
        if (generator_append_format(steps_text,
                                    sizeof(steps_text),
                                    &steps_cursor,
                                    "'%s'",
                                    specs[index].pipe_ops[step_index].arguments[argument_index]) != 0) {
          file_buffer_free(&source_app_js);
          return 1;
        }
      }
      if (generator_append_format(steps_text,
                                  sizeof(steps_text),
                                  &steps_cursor,
                                  "] }") != 0) {
        file_buffer_free(&source_app_js);
        return 1;
      }        
    }
    if (generator_append_format(app_js, sizeof(app_js), &cursor, "%s] });\n", steps_text) != 0) {
      file_buffer_free(&source_app_js);
      return 1;
    }
  }

  if (generator_append_text(app_js, sizeof(app_js), &cursor,
                            "  if (typeof window.ngInitializeApp === 'function') {\n"
                            "    window.ngInitializeApp(ngApp);\n"
                            "  }\n"
                            "  ngApp.start();\n"
                            "}());\n") != 0) {
    file_buffer_free(&source_app_js);
    return 1;
  }

  file_buffer_free(&source_app_js);
  LOG_TRACE("generator_emit_compiled_app_js bytes=%zu specs=%zu\n", cursor, spec_count);
  return generator_write_text_asset(output_dir, "app.js", app_js);
}

static int generator_emit_app_js(const char *output_dir, const char *input_dir, const ast_component_file_t *component) {
  char source_path[512];
  char destination_path[512];
  generator_observable_spec_t specs[GENERATOR_MAX_OBSERVABLES];
  size_t spec_count;

  generator_build_path(source_path, sizeof(source_path), input_dir, "app.js");
  generator_build_path(destination_path, sizeof(destination_path), output_dir, "app.js");
  spec_count = generator_collect_observable_specs(component, specs, GENERATOR_MAX_OBSERVABLES);
  LOG_TRACE("generator_emit_app_js source=%s destination=%s observable_count=%zu\n",
            source_path,
            destination_path,
            spec_count);
  if (spec_count > 0) {
    return generator_emit_compiled_app_js(output_dir, input_dir, component);
  }
  return output_fs_copy_file(source_path, destination_path);
}

static int generator_emit_makefile(const char *output_dir, const ast_component_file_t *component) {
  char path[512];
  char makefile_text[16384];
  size_t cursor = 0;
  size_t index;
  char safe_name[128];

  cursor += (size_t)snprintf(makefile_text + cursor,
                             sizeof(makefile_text) - cursor,
                             "SHELL := cmd.exe\n"
                             ".SHELLFLAGS := /C\n\n"
                             "CC := gcc\n"
                             "CFLAGS := -std=c11 -Wall -Wextra -Werror -I. -Ihelpers/include -Ihelpers/include/data -Ihelpers/include/support\n"
                             "LDFLAGS := -lm -lws2_32\n"
                             "BIN_DIR := bin\n"
                             "TARGET := $(BIN_DIR)/generated_demo.exe\n\n"
                             "SOURCES := \\\n");

  for (index = 0; index < component->member_count; ++index) {
    generator_make_safe_name(safe_name, sizeof(safe_name), component->members[index].name);
    cursor += (size_t)snprintf(makefile_text + cursor,
                               sizeof(makefile_text) - cursor,
                               "\tangular_%s.c \\\n",
                               safe_name);
  }

  cursor += (size_t)snprintf(makefile_text + cursor,
                             sizeof(makefile_text) - cursor,
                             "\tangular_http_service.c \\\n"
                             "\tangular_generated_demo.c \\\n"
                             "\thelpers/src/data/json.c \\\n"
                             "\thelpers/src/data/json_utils.c \\\n"
                             "\thelpers/src/format/number_format.c \\\n"
                             "\thelpers/src/io/fetch_runtime.c \\\n"
                             "\thelpers/src/math/ng_math.c \\\n"
                             "\thelpers/src/math/number_utils.c \\\n"
                             "\thelpers/src/net/http_server.c \\\n"
                             "\thelpers/src/net/http_service.c \\\n"
                             "\thelpers/src/runtime/app_runtime.c \\\n"
                             "\thelpers/src/runtime/observable.c \\\n"
                             "\thelpers/src/runtime/server_runtime.c \\\n"
                             "\thelpers/src/support/stringbuilder.c\n\n"
                             ".PHONY: all clean run\n\n"
                             "all: $(TARGET)\n\n"
                             "$(TARGET): $(SOURCES)\n"
                             "\tif not exist $(BIN_DIR) mkdir $(BIN_DIR)\n"
                             "\t$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS)\n\n"
                             "run: $(TARGET)\n"
                             "\t.\\\\$(subst /,\\\\,$(TARGET)) 18080 0\n\n"
                             "clean:\n"
                             "\tif exist $(BIN_DIR) rmdir /S /Q $(BIN_DIR)\n");

  generator_build_path(path, sizeof(path), output_dir, "Makefile");
  return output_fs_write_text(path, makefile_text);
}

static int generator_emit_member_header(const char *output_dir, const ast_component_file_t *component, const ast_member_t *member) {
  char filename[256];
  char path[512];
  char guard[256];
  char prototype[512];
  char header[4608];
  char safe_name[128];
  generator_member_spec_t spec;
  const char *member_kind = member->kind == AST_MEMBER_METHOD ? "method" : "field";
  int requires_external_fetch = member->uses_external_fetch;

  generator_fill_member_spec(member, &spec);

  LOG_TRACE("generator_emit_member_header member=%s kind=%s output_dir=%s\n",
            member->name,
            member_kind,
            output_dir);
  generator_make_safe_name(safe_name, sizeof(safe_name), member->name);

  if (spec.requires_external_fetch) {
    requires_external_fetch = 1;
  }

  snprintf(filename, sizeof(filename), "angular_%s.h", safe_name);
  generator_build_path(path, sizeof(path), output_dir, filename);
  generator_make_guard(guard, sizeof(guard), safe_name);
  generator_fill_member_prototype(component, member, prototype, sizeof(prototype));
  LOG_TRACE("generator_emit_member_header guard=%s path=%s requires_external_fetch=%d\n",
            guard,
            path,
            requires_external_fetch);

  snprintf(header,
           sizeof(header),
           "#ifndef %s\n"
           "#define %s\n\n"
           "#include \"angular_runtime.h\"\n\n"
           "#include \"helpers/include/io/fetch_runtime.h\"\n"
           "#include \"helpers/include/math/ng_math.h\"\n"
           "#include \"helpers/include/math/number_utils.h\"\n"
           "#include \"helpers/include/format/number_format.h\"\n"
           "#include \"helpers/include/runtime/app_runtime.h\"\n\n"
           "#define ANGULAR_%s_COMPONENT \"%s\"\n"
           "#define ANGULAR_%s_KIND \"%s\"\n"
           "#define ANGULAR_%s_RUNTIME_CATEGORY \"%s\"\n"
           "#define ANGULAR_%s_STORAGE_TYPE \"%s\"\n"
           "#define ANGULAR_%s_PROCESSING_NOTES \"%s\"\n"
           "#define ANGULAR_%s_REQUIRES_EXTERNAL_FETCH %d\n\n"
           "typedef struct {\n"
           "  const char *component_name;\n"
           "  const char *member_name;\n"
           "  const char *member_kind;\n"
           "  const char *runtime_category;\n"
           "  const char *storage_type;\n"
           "  const char *processing_notes;\n"
           "  int requires_external_fetch;\n"
           "} angular_%s_header_t;\n\n"
           "extern const angular_%s_header_t angular_%s_header;\n\n"
           "%s\n\n"
           "#endif\n",
           guard,
           guard,
           safe_name,
           component->class_name,
           member->name,
           member_kind,
           safe_name,
           spec.runtime_category,
           member->name,
           spec.storage_type,
           member->name,
           spec.processing_notes,
           safe_name,
           requires_external_fetch,
           safe_name,
           safe_name,
           safe_name,
           prototype);

  LOG_TRACE("generator_emit_member_header header_bytes=%zu member=%s\n", strlen(header), member->name);
  return output_fs_write_text(path, header);
}

int generator_prepare_output_directory(const char *output_dir) {
  LOG_TRACE("generator_prepare_output_directory output_dir=%s\n", output_dir);
  return output_fs_prepare_clean_directory(output_dir);
}

int generator_copy_runtime_header(const char *runtime_header_path, const char *output_dir) {
  char destination_path[512];

  LOG_TRACE("generator_copy_runtime_header runtime_header_path=%s output_dir=%s\n",
            runtime_header_path,
            output_dir);
  generator_build_path(destination_path, sizeof(destination_path), output_dir, "angular_runtime.h");
  return output_fs_copy_file(runtime_header_path, destination_path);
}

int generator_generate_component_headers(const char *output_dir, const ast_component_file_t *component) {
  size_t index;

  LOG_TRACE("generator_generate_component_headers class=%s member_count=%zu output_dir=%s\n",
            component->class_name,
            component->member_count,
            output_dir);

  for (index = 0; index < component->member_count; ++index) {
    LOG_TRACE("generator_generate_component_headers index=%zu member=%s\n",
              index,
              component->members[index].name);
    if (generator_emit_member_header(output_dir, component, &component->members[index]) != 0) {
      return 1;
    }
  }

  return 0;
}

int generator_validate_component_headers(const char *output_dir, const ast_component_file_t *component) {
  size_t index;
  char runtime_path[512];

  LOG_TRACE("generator_validate_component_headers class=%s output_dir=%s\n",
            component->class_name,
            output_dir);
  generator_build_path(runtime_path, sizeof(runtime_path), output_dir, "angular_runtime.h");
  if (!output_fs_file_exists(runtime_path)) {
    log_errorf("generated runtime header missing: %s\n", runtime_path);
    return 1;
  }

  for (index = 0; index < component->member_count; ++index) {
    char filename[256];
    char path[512];
    char safe_name[128];
    file_buffer_t buffer;
    generator_member_spec_t spec;

    generator_fill_member_spec(&component->members[index], &spec);
    generator_make_safe_name(safe_name, sizeof(safe_name), component->members[index].name);
    snprintf(filename, sizeof(filename), "angular_%s.h", safe_name);
    generator_build_path(path, sizeof(path), output_dir, filename);
    LOG_TRACE("generator_validate_component_headers checking path=%s\n", path);

    if (!output_fs_file_exists(path)) {
      log_errorf("generated header missing: %s\n", path);
      return 1;
    }

    if (file_read_all(path, &buffer) != 0) {
      log_errorf("failed to read generated header: %s\n", path);
      return 1;
    }

    if (strstr(buffer.data, "#include \"angular_runtime.h\"") == NULL ||
        strstr(buffer.data, component->class_name) == NULL ||
        strstr(buffer.data, component->members[index].name) == NULL ||
        strstr(buffer.data, spec.runtime_category) == NULL ||
        strstr(buffer.data, spec.storage_type) == NULL ||
        strstr(buffer.data, spec.processing_notes) == NULL) {
      file_buffer_free(&buffer);
      log_errorf("generated header content mismatch: %s\n", path);
      return 1;
    }

    LOG_TRACE("generator_validate_component_headers content_ok path=%s\n", path);
    file_buffer_free(&buffer);
  }

  log_printf("GENERATION VALIDATION OK\n");
  return 0;
}

int generator_generate_component_sources(const char *output_dir, const ast_component_file_t *component) {
  size_t index;

  LOG_TRACE("generator_generate_component_sources class=%s member_count=%zu output_dir=%s\n",
            component->class_name,
            component->member_count,
            output_dir);

  for (index = 0; index < component->member_count; ++index) {
    if (generator_emit_member_source(output_dir, component, &component->members[index]) != 0) {
      return 1;
    }
  }

  return 0;
}

int generator_validate_component_sources(const char *output_dir, const ast_component_file_t *component) {
  size_t index;

  for (index = 0; index < component->member_count; ++index) {
    char filename[256];
    char path[512];
    char safe_name[128];
    file_buffer_t buffer;

    generator_make_safe_name(safe_name, sizeof(safe_name), component->members[index].name);
    snprintf(filename, sizeof(filename), "angular_%s.c", safe_name);
    generator_build_path(path, sizeof(path), output_dir, filename);
    if (!output_fs_file_exists(path)) {
      log_errorf("generated source missing: %s\n", path);
      return 1;
    }

    if (file_read_all(path, &buffer) != 0) {
      log_errorf("failed to read generated source: %s\n", path);
      return 1;
    }

    if (strstr(buffer.data, component->members[index].name) == NULL) {
      file_buffer_free(&buffer);
      log_errorf("generated source content mismatch: %s\n", path);
      return 1;
    }

    file_buffer_free(&buffer);
  }

  log_printf("SOURCE GENERATION VALIDATION OK\n");
  return 0;
}

int generator_generate_demo_files(const char *output_dir,
                                  const char *input_dir,
                                  const ast_component_file_t *component,
                                  const char *html_source,
                                  const char *css_source) {
  generator_route_collection_t routes;
  generator_backend_assets_t *backend;
  size_t total_route_count;

  backend = (generator_backend_assets_t *)calloc(1, sizeof(*backend));
  if (backend == NULL) {
    return 1;
  }

  if (generator_collect_route_assets(input_dir, &routes) != 0) {
    free(backend);
    return 1;
  }
  if (generator_collect_backend_assets(input_dir, backend) != 0) {
    generator_free_route_assets(&routes);
    free(backend);
    return 1;
  }

  total_route_count = routes.route_count + backend->routes.route_count;
  if (generator_emit_http_service_header(output_dir, routes.route_count, total_route_count) != 0) {
    generator_free_route_assets(&routes);
    free(backend);
    return 1;
  }
  if (generator_emit_http_service_source(output_dir, &routes, backend) != 0) {
    generator_free_route_assets(&routes);
    free(backend);
    return 1;
  }
  if (generator_emit_demo_file(output_dir) != 0) {
    generator_free_route_assets(&routes);
    free(backend);
    return 1;
  }
  if (generator_emit_index_html(output_dir, component, html_source) != 0) {
    generator_free_route_assets(&routes);
    free(backend);
    return 1;
  }
  if (generator_emit_styles_css(output_dir, css_source) != 0) {
    generator_free_route_assets(&routes);
    free(backend);
    return 1;
  }
  if (generator_emit_app_js(output_dir, input_dir, component) != 0) {
    generator_free_route_assets(&routes);
    free(backend);
    return 1;
  }
  if (generator_emit_makefile(output_dir, component) != 0) {
    generator_free_route_assets(&routes);
    free(backend);
    return 1;
  }
  generator_free_route_assets(&routes);
  free(backend);
  return 0;
}

int generator_validate_demo_files(const char *output_dir) {
  size_t index;

  for (index = 0; index < sizeof(g_generated_demo_files) / sizeof(g_generated_demo_files[0]); ++index) {
    char path[512];

    generator_build_path(path, sizeof(path), output_dir, g_generated_demo_files[index]);
    if (!output_fs_file_exists(path)) {
      log_errorf("generated demo file missing: %s\n", path);
      return 1;
    }
  }

  log_printf("DEMO FILE VALIDATION OK\n");
  return 0;
}

int generator_copy_helpers(const char *helpers_root_path, const char *output_dir) {
  size_t index;

  LOG_TRACE("generator_copy_helpers helpers_root_path=%s output_dir=%s\n", helpers_root_path, output_dir);
  if (generator_prepare_helper_directories(output_dir) != 0) {
    return 1;
  }

  for (index = 0; index < sizeof(g_helper_files) / sizeof(g_helper_files[0]); ++index) {
    char source_path[512];
    char destination_path[512];
    char helper_root_output[512];

    generator_build_path(source_path, sizeof(source_path), helpers_root_path, g_helper_files[index]);
    generator_build_path(helper_root_output, sizeof(helper_root_output), output_dir, "helpers");
    generator_build_path(destination_path, sizeof(destination_path), helper_root_output, g_helper_files[index]);

    if (output_fs_copy_file(source_path, destination_path) != 0) {
      log_errorf("failed to copy helper file: %s\n", source_path);
      return 1;
    }
  }

  return 0;
}

int generator_validate_helpers(const char *output_dir) {
  size_t index;
  char helper_root_output[512];

  generator_build_path(helper_root_output, sizeof(helper_root_output), output_dir, "helpers");

  for (index = 0; index < sizeof(g_helper_files) / sizeof(g_helper_files[0]); ++index) {
    char destination_path[512];

    generator_build_path(destination_path, sizeof(destination_path), helper_root_output, g_helper_files[index]);
    if (!output_fs_file_exists(destination_path)) {
      log_errorf("copied helper missing: %s\n", destination_path);
      return 1;
    }
  }

  log_printf("HELPER COPY VALIDATION OK\n");
  return 0;
}
