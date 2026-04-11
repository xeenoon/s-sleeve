#include "generator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "asset_writer.h"
#include "file_io.h"
#include "log.h"
#include "output_fs.h"
#include "path_scan.h"

#define GENERATOR_MAX_ROUTES 32
#define GENERATOR_MAX_OBSERVABLES 16

typedef struct {
  char method[16];
  char path[256];
  char content_type[64];
  file_buffer_t body;
} generator_route_asset_t;

typedef struct {
  char routes_root[512];
  generator_route_asset_t routes[GENERATOR_MAX_ROUTES];
  size_t route_count;
} generator_route_collection_t;

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
  GENERATOR_PIPE_REDUCE
} generator_pipe_kind_t;

typedef struct {
  generator_pipe_kind_t kind;
  char argument[128];
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

static const char *g_generated_files[] = {
    "web_runtime_generated.h",
    "web_runtime_generated.cpp",
    "web_page_generated.cpp",
    "index.html",
    "styles.css",
    "app.js"};

static void generator_build_path(char *buffer,
                                 size_t buffer_size,
                                 const char *directory,
                                 const char *filename) {
  snprintf(buffer, buffer_size, "%s\\%s", directory, filename);
  LOG_TRACE("generator_build_path directory=%s filename=%s path=%s\n", directory, filename, buffer);
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

static void generator_escape_c_string(char *buffer, size_t buffer_size, const char *text) {
  size_t cursor = 0;
  size_t index;

  if (buffer_size == 0) {
    return;
  }

  for (index = 0; text != NULL && text[index] != '\0' && cursor + 2 < buffer_size; ++index) {
    char ch = text[index];
    switch (ch) {
      case '\\':
        buffer[cursor++] = '\\';
        buffer[cursor++] = '\\';
        break;
      case '"':
        buffer[cursor++] = '\\';
        buffer[cursor++] = '"';
        break;
      case '\n':
        buffer[cursor++] = '\\';
        buffer[cursor++] = 'n';
        break;
      case '\r':
        buffer[cursor++] = '\\';
        buffer[cursor++] = 'r';
        break;
      case '\t':
        buffer[cursor++] = '\\';
        buffer[cursor++] = 't';
        break;
      default:
        buffer[cursor++] = ch;
        break;
    }
  }

  buffer[cursor] = '\0';
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

  while (begin < length &&
         (start[begin] == ' ' || start[begin] == '\t' || start[begin] == '\r' || start[begin] == '\n')) {
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

static int generator_parse_numeric_argument_after_quote(const char *text,
                                                        const char *prefix,
                                                        int *out_value) {
  const char *start = strstr(text, prefix);
  const char *cursor;
  int depth = 1;
  int seen_quote = 0;
  char number_buffer[32];
  size_t number_length = 0;

  if (out_value == NULL || start == NULL) {
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
    return generator_parse_quoted_argument(segment, "rx.prop(", op->argument, sizeof(op->argument));
  }
  if (strstr(segment, "rx.map(") != NULL) {
    op->kind = GENERATOR_PIPE_MAP;
    return generator_parse_quoted_argument(segment, "rx.map(", op->argument, sizeof(op->argument));
  }
  if (strstr(segment, "rx.tap(") != NULL) {
    op->kind = GENERATOR_PIPE_TAP;
    return generator_parse_quoted_argument(segment, "rx.tap(", op->argument, sizeof(op->argument));
  }
  if (strstr(segment, "rx.reduce(") != NULL) {
    op->kind = GENERATOR_PIPE_REDUCE;
    return generator_parse_quoted_argument(segment, "rx.reduce(", op->argument, sizeof(op->argument));
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

static size_t generator_collect_observable_specs(const ast_component_file_t *component,
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
    case GENERATOR_PIPE_UNKNOWN:
    default:
      return "unknown";
  }
}

static int generator_append_text(char *buffer, size_t buffer_size, size_t *cursor, const char *text) {
  size_t text_length = strlen(text);

  if (*cursor + text_length + 1 >= buffer_size) {
    return 1;
  }

  memcpy(buffer + *cursor, text, text_length);
  *cursor += text_length;
  buffer[*cursor] = '\0';
  return 0;
}

static int generator_append_format(char *buffer, size_t buffer_size, size_t *cursor, const char *format, ...) {
  va_list args;
  int written;

  va_start(args, format);
  written = vsnprintf(buffer + *cursor, buffer_size - *cursor, format, args);
  va_end(args);

  if (written < 0 || *cursor + (size_t)written + 1 >= buffer_size) {
    return 1;
  }

  *cursor += (size_t)written;
  return 0;
}

static int generator_emit_compiled_app_js(const char *output_dir,
                                          const char *input_dir,
                                          const ast_component_file_t *component) {
  char source_path[512];
  char app_js[262144];
  size_t cursor = 0;
  file_buffer_t source_app_js;
  generator_observable_spec_t specs[GENERATOR_MAX_OBSERVABLES];
  size_t spec_count;
  size_t index;

  memset(&source_app_js, 0, sizeof(source_app_js));
  generator_build_path(source_path, sizeof(source_path), input_dir, "app.js");
  spec_count = generator_collect_observable_specs(component, specs, GENERATOR_MAX_OBSERVABLES);
  LOG_TRACE("generator_emit_compiled_app_js spec_count=%zu source=%s\n", spec_count, source_path);

  if (file_read_all(source_path, &source_app_js) != 0) {
    log_errorf("failed to read app.js source: %s\n", source_path);
    return 1;
  }

  app_js[0] = '\0';
  if (generator_append_text(app_js, sizeof(app_js), &cursor,
                            "(function () {\n"
                            "  function ngLog() {\n"
                            "    var args = Array.prototype.slice.call(arguments);\n"
                            "    args.unshift('[ng-runtime]');\n"
                            "    console.log.apply(console, args);\n"
                            "  }\n\n"
                            "  function ngFetchJson(path, options) {\n"
                            "    ngLog('fetch', path, options && options.method ? options.method : 'GET');\n"
                            "    return fetch(path, options).then(function (response) {\n"
                            "      return response.json();\n"
                            "    });\n"
                            "  }\n\n"
                            "  function ngResolveHook(name) {\n"
                            "    return typeof window[name] === 'function' ? window[name] : null;\n"
                            "  }\n\n"
                            "  function ngNormalizeName(name) {\n"
                            "    return name && name.charAt(name.length - 1) === '$' ? name.slice(0, -1) : name;\n"
                            "  }\n\n"
                            "  function ngApplyPipeChain(value, steps, app, spec) {\n"
                            "    return (steps || []).reduce(function (current, step) {\n"
                            "      var hook;\n"
                            "      ngLog('pipe', spec.name, step.kind, step.argument);\n"
                            "      if (step.kind === 'prop') {\n"
                            "        if (current && typeof current === 'object') {\n"
                            "          return current[step.argument];\n"
                            "        }\n"
                            "        return undefined;\n"
                            "      }\n"
                            "      if (step.kind === 'map') {\n"
                            "        hook = ngResolveHook(step.argument);\n"
                            "        if (!hook) {\n"
                            "          ngLog('missing map hook', step.argument);\n"
                            "          return current;\n"
                            "        }\n"
                            "        if (Array.isArray(current)) {\n"
                            "          return current.map(function (item, itemIndex) { return hook(item, app, itemIndex, spec); });\n"
                            "        }\n"
                            "        return hook(current, app, 0, spec);\n"
                            "      }\n"
                            "      if (step.kind === 'reduce') {\n"
                            "        hook = ngResolveHook(step.argument);\n"
                            "        if (!hook) {\n"
                            "          ngLog('missing reduce hook', step.argument);\n"
                            "          return current;\n"
                            "        }\n"
                            "        return hook(current, app, spec);\n"
                            "      }\n"
                            "      if (step.kind === 'tap') {\n"
                            "        hook = ngResolveHook(step.argument);\n"
                            "        if (hook) {\n"
                            "          hook(current, app, spec);\n"
                            "        } else {\n"
                            "          ngLog('missing tap hook', step.argument);\n"
                            "        }\n"
                            "        return current;\n"
                            "      }\n"
                            "      return current;\n"
                            "    }, value);\n"
                            "  }\n\n"
                            "  function ngCreateApp() {\n"
                            "    var app = {\n"
                            "      streams: {},\n"
                            "      values: {},\n"
                            "      intervals: []\n"
                            "    };\n\n"
                            "    app.getValue = function (name) {\n"
                            "      return app.values[name];\n"
                            "    };\n\n"
                            "    app.setValue = function (name, value) {\n"
                            "      ngLog('setValue', name, value);\n"
                            "      app.values[name] = value;\n"
                            "      return value;\n"
                            "    };\n\n"
                            "    app.applyObservableValue = function (spec, rawValue) {\n"
                            "      var processed = ngApplyPipeChain(rawValue, spec.steps, app, spec);\n"
                            "      var alias = ngNormalizeName(spec.name);\n"
                            "      ngLog('applyObservableValue', spec.name, alias, processed);\n"
                            "      app.values[spec.name] = processed;\n"
                            "      app.values[alias] = processed;\n"
                            "      return processed;\n"
                            "    };\n\n"
                            "    app.registerObservable = function (spec) {\n"
                            "      ngLog('registerObservable', spec.name, spec.kind, spec.path || spec.seed || '');\n"
                            "      app.streams[spec.name] = spec;\n"
                            "      if (spec.kind === 'state') {\n"
                            "        app.applyObservableValue(spec, spec.seed);\n"
                            "      }\n"
                            "    };\n\n"
                            "    app.setObservable = function (name, value) {\n"
                            "      var spec = app.streams[name];\n"
                            "      if (!spec || spec.kind !== 'state') {\n"
                            "        ngLog('setObservable ignored', name);\n"
                            "        return;\n"
                            "      }\n"
                            "      app.applyObservableValue(spec, value);\n"
                            "    };\n\n"
                            "    app.refreshObservable = function (name) {\n"
                            "      var spec = app.streams[name];\n"
                            "      if (!spec || spec.kind !== 'poll') {\n"
                            "        ngLog('refreshObservable ignored', name);\n"
                            "        return Promise.resolve(null);\n"
                            "      }\n"
                            "      return ngFetchJson(spec.path).then(function (payload) {\n"
                            "        return app.applyObservableValue(spec, payload);\n"
                            "      });\n"
                            "    };\n\n"
                            "    app.runObservable = function (name, payload) {\n"
                            "      var spec = app.streams[name];\n"
                            "      if (!spec || spec.kind !== 'post') {\n"
                            "        ngLog('runObservable ignored', name);\n"
                            "        return Promise.resolve(null);\n"
                            "      }\n"
                            "      return ngFetchJson(spec.path, {\n"
                            "        method: 'POST',\n"
                            "        headers: { 'Content-Type': 'application/json' },\n"
                            "        body: JSON.stringify(payload || {})\n"
                            "      }).then(function (responsePayload) {\n"
                            "        return app.applyObservableValue(spec, responsePayload);\n"
                            "      });\n"
                            "    };\n\n"
                            "    app.start = function () {\n"
                            "      Object.keys(app.streams).forEach(function (name) {\n"
                            "        var spec = app.streams[name];\n"
                            "        if (spec.kind === 'poll') {\n"
                            "          app.refreshObservable(name);\n"
                            "          app.intervals.push(window.setInterval(function () {\n"
                            "            app.refreshObservable(name);\n"
                            "          }, spec.intervalMs));\n"
                            "        }\n"
                            "      });\n"
                            "    };\n\n"
                            "    return app;\n"
                            "  }\n\n") != 0) {
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
                                  "{ kind: '%s', argument: '%s' }",
                                  generator_pipe_kind_name(specs[index].pipe_ops[step_index].kind),
                                  specs[index].pipe_ops[step_index].argument) != 0) {
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

static const char *generator_content_type_for_extension(const char *path) {
  const char *last_dot = strrchr(path, '.');

  if (last_dot == NULL) {
    return "text/plain; charset=utf-8";
  }
  if (strcmp(last_dot, ".json") == 0) {
    return "application/json";
  }
  if (strcmp(last_dot, ".html") == 0) {
    return "text/html; charset=utf-8";
  }
  if (strcmp(last_dot, ".js") == 0) {
    return "application/javascript; charset=utf-8";
  }
  if (strcmp(last_dot, ".css") == 0) {
    return "text/css; charset=utf-8";
  }
  return "text/plain; charset=utf-8";
}

static int generator_route_path_from_relative(const char *relative_path,
                                              char *method,
                                              size_t method_size,
                                              char *route_path,
                                              size_t route_path_size) {
  const char *cursor = relative_path;
  const char *separator = strchr(cursor, '\\');
  const char *filename;
  const char *last_dot;
  size_t prefix_length;
  size_t cursor_index = 0;

  if (separator == NULL) {
    separator = strchr(cursor, '/');
  }
  if (separator == NULL) {
    return 1;
  }

  prefix_length = (size_t)(separator - cursor);
  if (prefix_length + 1 > method_size) {
    return 1;
  }
  memcpy(method, cursor, prefix_length);
  method[prefix_length] = '\0';

  cursor = separator + 1;
  filename = strrchr(cursor, '\\');
  if (filename == NULL) {
    filename = strrchr(cursor, '/');
  }
  filename = filename != NULL ? filename + 1 : cursor;
  last_dot = strrchr(filename, '.');
  if (last_dot == NULL) {
    return 1;
  }

  if (route_path_size < 2) {
    return 1;
  }
  route_path[cursor_index++] = '/';

  while (*cursor != '\0' && cursor_index + 2 < route_path_size) {
    char ch = *cursor++;
    if (ch == '\\' || ch == '/') {
      route_path[cursor_index++] = '/';
      continue;
    }
    if (ch == '.' && cursor - 1 == last_dot) {
      break;
    }
    route_path[cursor_index++] = ch;
  }

  if (cursor_index > 1) {
    size_t end = cursor_index;
    while (end > 1 && route_path[end - 1] == '/') {
      end -= 1;
    }
    cursor_index = end;
  }

  route_path[cursor_index] = '\0';
  return 0;
}

static int generator_collect_route_file(const char *path, void *context) {
  generator_route_collection_t *routes = (generator_route_collection_t *)context;
  const char *relative_path;
  file_buffer_t buffer;
  generator_route_asset_t *route;
  size_t root_length;

  if (routes->route_count >= GENERATOR_MAX_ROUTES) {
    log_errorf("too many embedded static routes, max=%d\n", GENERATOR_MAX_ROUTES);
    return 1;
  }

  root_length = strlen(routes->routes_root);
  if (_strnicmp(path, routes->routes_root, root_length) != 0) {
    return 0;
  }

  relative_path = path + root_length;
  while (*relative_path == '\\' || *relative_path == '/') {
    relative_path += 1;
  }

  if (file_read_all(path, &buffer) != 0) {
    log_errorf("failed to read embedded route asset: %s\n", path);
    return 1;
  }

  route = &routes->routes[routes->route_count];
  memset(route, 0, sizeof(*route));
  if (generator_route_path_from_relative(relative_path,
                                         route->method,
                                         sizeof(route->method),
                                         route->path,
                                         sizeof(route->path)) != 0) {
    file_buffer_free(&buffer);
    log_errorf("failed to derive route path from asset: %s\n", path);
    return 1;
  }

  snprintf(route->content_type, sizeof(route->content_type), "%s", generator_content_type_for_extension(path));
  route->body = buffer;
  routes->route_count += 1;
  LOG_TRACE("generator_collect_route_file method=%s path=%s bytes=%zu\n",
            route->method,
            route->path,
            route->body.size);
  return 0;
}

static int generator_collect_route_assets(const char *input_dir, generator_route_collection_t *routes) {
  char routes_root[512];

  memset(routes, 0, sizeof(*routes));
  generator_build_path(routes_root, sizeof(routes_root), input_dir, "routes");
  snprintf(routes->routes_root, sizeof(routes->routes_root), "%s", routes_root);

  if (!output_fs_file_exists(routes_root)) {
    LOG_TRACE("generator_collect_route_assets no routes directory=%s\n", routes_root);
    return 0;
  }

  LOG_TRACE("generator_collect_route_assets root=%s\n", routes_root);
  return path_scan_directory(routes_root, generator_collect_route_file, routes);
}

static void generator_free_route_assets(generator_route_collection_t *routes) {
  size_t index;
  for (index = 0; index < routes->route_count; ++index) {
    file_buffer_free(&routes->routes[index].body);
  }
  routes->route_count = 0;
}

static int generator_emit_cpp_bundle(const char *output_dir,
                                     const char *html_source,
                                     const char *css_source,
                                     const char *js_source) {
  char path[512];
  FILE *file;

  generator_build_path(path, sizeof(path), output_dir, "web_page_generated.cpp");
  LOG_TRACE("generator_emit_cpp_bundle path=%s\n", path);

  file = fopen(path, "wb");
  if (file == NULL) {
    log_errorf("failed to open generated bundle for write: %s\n", path);
    return 1;
  }

  if (asset_writer_write_prolog(file) != 0 ||
      asset_writer_write_asset(file, "INDEX_HTML", "HTML_ASSET", html_source) != 0 ||
      asset_writer_write_asset(file, "STYLES_CSS", "CSS_ASSET", css_source) != 0 ||
      asset_writer_write_asset(file, "APP_JS", "JS_ASSET", js_source) != 0 ||
      asset_writer_write_epilog(file) != 0) {
    fclose(file);
    log_errorf("failed to write generated C++ web bundle: %s\n", path);
    return 1;
  }

  fclose(file);
  return 0;
}

static int generator_emit_runtime_header(const char *output_dir) {
  const char *header_text =
      "#ifndef GENERATED_WEB_RUNTIME_H\n"
      "#define GENERATED_WEB_RUNTIME_H\n\n"
      "#include <Arduino.h>\n"
      "#include <WebServer.h>\n\n"
      "typedef struct {\n"
      "  const char *method;\n"
      "  const char *path;\n"
      "  const char *content_type;\n"
      "  PGM_P body;\n"
      "} generated_web_static_route_t;\n\n"
      "void generated_web_send_root(WebServer &server);\n"
      "void generated_web_send_styles(WebServer &server);\n"
      "void generated_web_send_app_js(WebServer &server);\n"
      "bool generated_web_try_send_static_route(WebServer &server, const String &path, HTTPMethod method);\n"
      "size_t generated_web_static_route_count(void);\n"
      "const generated_web_static_route_t *generated_web_static_route_at(size_t index);\n\n"
      "#endif\n";

  return generator_write_text_asset(output_dir, "web_runtime_generated.h", header_text);
}

static int generator_emit_runtime_source(const char *output_dir,
                                         const generator_route_collection_t *routes) {
  char source_text[98304];
  char route_bodies[49152] = "";
  char route_table[16384] = "";
  size_t body_cursor = 0;
  size_t table_cursor = 0;
  size_t index;

  for (index = 0; index < routes->route_count; ++index) {
    char escaped_body[8192];
    generator_escape_c_string(escaped_body, sizeof(escaped_body), routes->routes[index].body.data);
    body_cursor += (size_t)snprintf(route_bodies + body_cursor,
                                    sizeof(route_bodies) - body_cursor,
                                    "static const char g_route_body_%zu[] PROGMEM = \"%s\";\n",
                                    index,
                                    escaped_body);
    table_cursor += (size_t)snprintf(route_table + table_cursor,
                                     sizeof(route_table) - table_cursor,
                                     "  { \"%s\", \"%s\", \"%s\", g_route_body_%zu },\n",
                                     routes->routes[index].method,
                                     routes->routes[index].path,
                                     routes->routes[index].content_type,
                                     index);
  }

  snprintf(source_text,
           sizeof(source_text),
           "#include <Arduino.h>\n"
           "#include <WebServer.h>\n\n"
           "#include <string.h>\n\n"
           "#include \"generated/web_runtime_generated.h\"\n"
           "#include \"web_page.h\"\n\n"
           "%s\n"
           "static const generated_web_static_route_t g_generated_routes[] = {\n"
           "%s"
           "};\n\n"
           "static const char *generated_web_method_name(HTTPMethod method) {\n"
           "  switch (method) {\n"
           "    case HTTP_GET: return \"GET\";\n"
           "    case HTTP_POST: return \"POST\";\n"
           "    case HTTP_PUT: return \"PUT\";\n"
           "    case HTTP_DELETE: return \"DELETE\";\n"
           "    case HTTP_PATCH: return \"PATCH\";\n"
           "    case HTTP_OPTIONS: return \"OPTIONS\";\n"
           "    default: return \"GET\";\n"
           "  }\n"
           "}\n\n"
           "void generated_web_send_root(WebServer &server) {\n"
           "  server.send_P(200, \"text/html\", INDEX_HTML);\n"
           "}\n\n"
           "void generated_web_send_styles(WebServer &server) {\n"
           "  server.send_P(200, \"text/css\", STYLES_CSS);\n"
           "}\n\n"
           "void generated_web_send_app_js(WebServer &server) {\n"
           "  server.send_P(200, \"application/javascript\", APP_JS);\n"
           "}\n\n"
           "size_t generated_web_static_route_count(void) {\n"
           "  return sizeof(g_generated_routes) / sizeof(g_generated_routes[0]);\n"
           "}\n\n"
           "const generated_web_static_route_t *generated_web_static_route_at(size_t index) {\n"
           "  if (index >= generated_web_static_route_count()) {\n"
           "    return nullptr;\n"
           "  }\n"
           "  return &g_generated_routes[index];\n"
           "}\n\n"
           "bool generated_web_try_send_static_route(WebServer &server, const String &path, HTTPMethod method) {\n"
           "  const char *method_name = generated_web_method_name(method);\n"
           "  size_t index;\n"
           "  for (index = 0; index < generated_web_static_route_count(); ++index) {\n"
           "    const generated_web_static_route_t &route = g_generated_routes[index];\n"
           "    if (strcmp(route.method, method_name) == 0 && path.equals(route.path)) {\n"
           "      server.send_P(200, route.content_type, route.body);\n"
           "      return true;\n"
           "    }\n"
           "  }\n"
           "  return false;\n"
           "}\n",
           route_bodies,
           route_table);

  return generator_write_text_asset(output_dir, "web_runtime_generated.cpp", source_text);
}

int generator_prepare_output_directory(const char *output_dir) {
  LOG_TRACE("generator_prepare_output_directory output_dir=%s\n", output_dir);
  return output_fs_prepare_clean_directory(output_dir);
}

int generator_generate_embedded_bundle(const char *output_dir,
                                       const char *input_dir,
                                       const ast_component_file_t *component,
                                       const char *html_source,
                                       const char *css_source) {
  file_buffer_t js_buffer;
  char generated_app_js_path[512];
  generator_route_collection_t routes;

  memset(&js_buffer, 0, sizeof(js_buffer));

  if (generator_collect_route_assets(input_dir, &routes) != 0) {
    return 1;
  }

  if (generator_emit_app_js(output_dir, input_dir, component) != 0) {
    generator_free_route_assets(&routes);
    return 1;
  }

  generator_build_path(generated_app_js_path, sizeof(generated_app_js_path), output_dir, "app.js");
  if (file_read_all(generated_app_js_path, &js_buffer) != 0) {
    log_errorf("failed to read generated embedded app.js source: %s\n", generated_app_js_path);
    generator_free_route_assets(&routes);
    return 1;
  }

  if (generator_emit_runtime_header(output_dir) != 0 ||
      generator_emit_runtime_source(output_dir, &routes) != 0 ||
      generator_emit_cpp_bundle(output_dir, html_source, css_source, js_buffer.data) != 0 ||
      generator_write_text_asset(output_dir, "index.html", html_source) != 0 ||
      generator_write_text_asset(output_dir, "styles.css", css_source) != 0) {
    generator_free_route_assets(&routes);
    file_buffer_free(&js_buffer);
    return 1;
  }

  generator_free_route_assets(&routes);
  file_buffer_free(&js_buffer);
  return 0;
}

int generator_validate_embedded_bundle(const char *output_dir) {
  size_t index;

  for (index = 0; index < sizeof(g_generated_files) / sizeof(g_generated_files[0]); ++index) {
    char path[512];
    generator_build_path(path, sizeof(path), output_dir, g_generated_files[index]);
    if (!output_fs_file_exists(path)) {
      log_errorf("generated embedded file missing: %s\n", path);
      return 1;
    }
  }

  log_printf("EMBEDDED BUNDLE VALIDATION OK\n");
  return 0;
}
