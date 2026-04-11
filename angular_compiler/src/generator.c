#include "generator.h"

#include <stdio.h>
#include <string.h>

#include "file_io.h"
#include "js_codegen.h"
#include "log.h"
#include "output_fs.h"

typedef struct {
  const char *runtime_category;
  const char *storage_type;
  const char *processing_notes;
  int requires_external_fetch;
} generator_member_spec_t;

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

static const generator_member_spec_t *generator_lookup_spec(const ast_member_t *member) {
  static const generator_member_spec_t reading = {
      "state-slot",
      "int",
      "raw potentiometer reading mirrored into ng_app_state_t.reading",
      0};
  static const generator_member_spec_t percent_straight = {
      "derived-state",
      "int",
      "derived percentage from normalized knee angle",
      0};
  static const generator_member_spec_t has_signal = {
      "status-flag",
      "bool",
      "tracks whether the latest sensor refresh succeeded",
      0};
  static const generator_member_spec_t attribute_text = {
      "attribute-buffer",
      "char buffer[16]",
      "formatted attribute payload for SVG bindings",
      0};
  static const generator_member_spec_t ng_on_init = {
      "lifecycle-hook",
      "ng_init_fn_t",
      "initializes timers and performs the first render-oriented state setup",
      0};
  static const generator_member_spec_t clamp = {
      "helper-function",
      "float helper",
      "pure bounds clamp helper used by derived calculations",
      0};
  static const generator_member_spec_t map_reading = {
      "helper-function",
      "derived mapping helper",
      "maps the sensor domain into normalized and angle values",
      0};
  static const generator_member_spec_t update_knee = {
      "recompute-hook",
      "ng_recompute_fn_t-compatible helper",
      "recomputes leg geometry and derived display state",
      0};
  static const generator_member_spec_t refresh_reading = {
      "io-effect",
      "external fetch effect",
      "assumes an existing C fetch hook is provided by the destination project",
      1};
  static const generator_member_spec_t generic_field = {
      "generic-field",
      "opaque generated field",
      "generated fallback metadata for unsupported component fields",
      0};
  static const generator_member_spec_t generic_method = {
      "generic-method",
      "opaque generated method",
      "generated fallback metadata for unsupported component methods",
      0};

  if (strcmp(member->name, "reading") == 0) {
    return &reading;
  }
  if (strcmp(member->name, "percentStraight") == 0) {
    return &percent_straight;
  }
  if (strcmp(member->name, "hasSignal") == 0) {
    return &has_signal;
  }
  if (strcmp(member->name, "lowerLegX2") == 0 ||
      strcmp(member->name, "lowerLegY2") == 0 ||
      strcmp(member->name, "ankleX") == 0 ||
      strcmp(member->name, "ankleY") == 0) {
    return &attribute_text;
  }
  if (strcmp(member->name, "ngOnInit") == 0) {
    return &ng_on_init;
  }
  if (strcmp(member->name, "clamp") == 0) {
    return &clamp;
  }
  if (strcmp(member->name, "mapReadingToAngle") == 0) {
    return &map_reading;
  }
  if (strcmp(member->name, "updateKnee") == 0) {
    return &update_knee;
  }
  if (strcmp(member->name, "refreshReading") == 0) {
    return &refresh_reading;
  }

  if (member->kind == AST_MEMBER_METHOD) {
    return &generic_method;
  }

  return &generic_field;
}

static void generator_fill_member_prototype(const ast_component_file_t *component,
                                            const ast_member_t *member,
                                            char *buffer,
                                            size_t buffer_size) {
  js_codegen_result_t codegen;

  if (strcmp(member->name, "reading") == 0) {
    snprintf(buffer, buffer_size, "int angular_reading_get(const ng_runtime_t *runtime);");
    return;
  }
  if (strcmp(member->name, "percentStraight") == 0) {
    snprintf(buffer, buffer_size, "int angular_percentStraight_get(const ng_runtime_t *runtime);");
    return;
  }
  if (strcmp(member->name, "hasSignal") == 0) {
    snprintf(buffer, buffer_size, "bool angular_hasSignal_get(const ng_runtime_t *runtime);");
    return;
  }
  if (strcmp(member->name, "lowerLegX2") == 0) {
    snprintf(buffer, buffer_size, "const char *angular_lowerLegX2_get(const ng_runtime_t *runtime);");
    return;
  }
  if (strcmp(member->name, "lowerLegY2") == 0) {
    snprintf(buffer, buffer_size, "const char *angular_lowerLegY2_get(const ng_runtime_t *runtime);");
    return;
  }
  if (strcmp(member->name, "ankleX") == 0) {
    snprintf(buffer, buffer_size, "const char *angular_ankleX_get(const ng_runtime_t *runtime);");
    return;
  }
  if (strcmp(member->name, "ankleY") == 0) {
    snprintf(buffer, buffer_size, "const char *angular_ankleY_get(const ng_runtime_t *runtime);");
    return;
  }
  if (strcmp(member->name, "ngOnInit") == 0) {
    snprintf(buffer, buffer_size, "void angular_ngOnInit_call(ng_runtime_t *runtime);");
    return;
  }
  if (strcmp(member->name, "clamp") == 0) {
    snprintf(buffer, buffer_size, "double angular_clamp_call(ng_runtime_t *runtime, double value, double min_value, double max_value);");
    return;
  }
  if (strcmp(member->name, "mapReadingToAngle") == 0) {
    snprintf(buffer, buffer_size, "ng_angle_map_t angular_mapReadingToAngle_call(int reading);");
    return;
  }
  if (strcmp(member->name, "updateKnee") == 0) {
    snprintf(buffer, buffer_size, "void angular_updateKnee_call(ng_runtime_t *runtime, double percentStraight);");
    return;
  }
  if (strcmp(member->name, "refreshReading") == 0) {
    snprintf(buffer,
             buffer_size,
             "int angular_refreshReading_call(ng_runtime_t *runtime, ng_fetch_text_fn fetch_fn, void *fetch_context);");
    return;
  }

  if (member->kind == AST_MEMBER_METHOD && js_codegen_compile_method(component, member, &codegen) == 0 && codegen.supported) {
    snprintf(buffer, buffer_size, "%s", codegen.prototype);
    return;
  }

  snprintf(buffer, buffer_size, "void angular_%s_generated_stub(void);", member->name);
}

static void generator_fill_member_definition(const ast_component_file_t *component,
                                             const ast_member_t *member,
                                             char *buffer,
                                             size_t buffer_size) {
  if (strcmp(member->name, "reading") == 0) {
    snprintf(buffer,
             buffer_size,
             "const angular_reading_header_t angular_reading_header = {\n"
             "  \"%s\",\n"
             "  \"reading\",\n"
             "  \"field\",\n"
             "  \"state-slot\",\n"
             "  \"int\",\n"
             "  \"raw potentiometer reading mirrored into ng_app_state_t.reading\",\n"
             "  0\n"
             "};\n\n"
             "int angular_reading_get(const ng_runtime_t *runtime) {\n"
             "  return ng_runtime_get_int(runtime, \"reading\", 0);\n"
             "}\n",
             component->class_name);
    return;
  }
  if (strcmp(member->name, "percentStraight") == 0) {
    snprintf(buffer,
             buffer_size,
             "const angular_percentStraight_header_t angular_percentStraight_header = {\n"
             "  \"%s\",\n"
             "  \"percentStraight\",\n"
             "  \"field\",\n"
             "  \"derived-state\",\n"
             "  \"int\",\n"
             "  \"derived percentage from normalized knee angle\",\n"
             "  0\n"
             "};\n\n"
             "int angular_percentStraight_get(const ng_runtime_t *runtime) {\n"
             "  return ng_runtime_get_int(runtime, \"percentStraight\", 0);\n"
             "}\n",
             component->class_name);
    return;
  }
  if (strcmp(member->name, "hasSignal") == 0) {
    snprintf(buffer,
             buffer_size,
             "const angular_hasSignal_header_t angular_hasSignal_header = {\n"
             "  \"%s\",\n"
             "  \"hasSignal\",\n"
             "  \"field\",\n"
             "  \"status-flag\",\n"
             "  \"bool\",\n"
             "  \"tracks whether the latest sensor refresh succeeded\",\n"
             "  0\n"
             "};\n\n"
             "bool angular_hasSignal_get(const ng_runtime_t *runtime) {\n"
             "  return ng_runtime_get_bool(runtime, \"hasSignal\", 0) != 0;\n"
             "}\n",
             component->class_name);
    return;
  }
  if (strcmp(member->name, "lowerLegX2") == 0) {
    snprintf(buffer,
             buffer_size,
             "const angular_lowerLegX2_header_t angular_lowerLegX2_header = {\n"
             "  \"%s\",\n"
             "  \"lowerLegX2\",\n"
             "  \"field\",\n"
             "  \"attribute-buffer\",\n"
             "  \"char buffer[16]\",\n"
             "  \"formatted attribute payload for SVG bindings\",\n"
             "  0\n"
             "};\n\n"
             "const char *angular_lowerLegX2_get(const ng_runtime_t *runtime) {\n"
             "  return ng_runtime_get_string(runtime, \"lowerLegX2\", \"140.0\");\n"
             "}\n",
             component->class_name);
    return;
  }
  if (strcmp(member->name, "lowerLegY2") == 0) {
    snprintf(buffer,
             buffer_size,
             "const angular_lowerLegY2_header_t angular_lowerLegY2_header = {\n"
             "  \"%s\",\n"
             "  \"lowerLegY2\",\n"
             "  \"field\",\n"
             "  \"attribute-buffer\",\n"
             "  \"char buffer[16]\",\n"
             "  \"formatted attribute payload for SVG bindings\",\n"
             "  0\n"
             "};\n\n"
             "const char *angular_lowerLegY2_get(const ng_runtime_t *runtime) {\n"
             "  return ng_runtime_get_string(runtime, \"lowerLegY2\", \"274.0\");\n"
             "}\n",
             component->class_name);
    return;
  }
  if (strcmp(member->name, "ankleX") == 0) {
    snprintf(buffer,
             buffer_size,
             "const angular_ankleX_header_t angular_ankleX_header = {\n"
             "  \"%s\",\n"
             "  \"ankleX\",\n"
             "  \"field\",\n"
             "  \"attribute-buffer\",\n"
             "  \"char buffer[16]\",\n"
             "  \"formatted attribute payload for SVG bindings\",\n"
             "  0\n"
             "};\n\n"
             "const char *angular_ankleX_get(const ng_runtime_t *runtime) {\n"
             "  return ng_runtime_get_string(runtime, \"ankleX\", \"140.0\");\n"
             "}\n",
             component->class_name);
    return;
  }
  if (strcmp(member->name, "ankleY") == 0) {
    snprintf(buffer,
             buffer_size,
             "const angular_ankleY_header_t angular_ankleY_header = {\n"
             "  \"%s\",\n"
             "  \"ankleY\",\n"
             "  \"field\",\n"
             "  \"attribute-buffer\",\n"
             "  \"char buffer[16]\",\n"
             "  \"formatted attribute payload for SVG bindings\",\n"
             "  0\n"
             "};\n\n"
             "const char *angular_ankleY_get(const ng_runtime_t *runtime) {\n"
             "  return ng_runtime_get_string(runtime, \"ankleY\", \"274.0\");\n"
             "}\n",
             component->class_name);
    return;
  }
  if (strcmp(member->name, "ngOnInit") == 0) {
    snprintf(buffer,
             buffer_size,
             "const angular_ngOnInit_header_t angular_ngOnInit_header = {\n"
             "  \"%s\",\n"
             "  \"ngOnInit\",\n"
             "  \"method\",\n"
             "  \"lifecycle-hook\",\n"
             "  \"ng_init_fn_t\",\n"
             "  \"initializes timers and performs the first render-oriented state setup\",\n"
             "  0\n"
             "};\n\n"
             "void angular_ngOnInit_call(ng_runtime_t *runtime) {\n"
             "  ng_runtime_init(runtime);\n"
             "  ng_runtime_set_string(runtime, \"selectedView\", \"live\");\n"
             "  ng_runtime_set_int(runtime, \"goal\", 85);\n"
             "  ng_runtime_set_string(runtime, \"lowerLegX2\", \"140.0\");\n"
             "  ng_runtime_set_string(runtime, \"lowerLegY2\", \"274.0\");\n"
             "  ng_runtime_set_string(runtime, \"ankleX\", \"140.0\");\n"
             "  ng_runtime_set_string(runtime, \"ankleY\", \"274.0\");\n"
             "  ng_runtime_set_bool(runtime, \"hasSignal\", 0);\n"
             "}\n",
             component->class_name);
    return;
  }
  if (strcmp(member->name, "clamp") == 0 ||
      strcmp(member->name, "mapPercentToAngle") == 0 ||
      strcmp(member->name, "updateKnee") == 0) {
    js_codegen_result_t codegen;
    if (js_codegen_compile_method(component, member, &codegen) == 0 && codegen.supported) {
      snprintf(buffer,
               buffer_size,
               "const angular_%s_header_t angular_%s_header = {\n"
               "  \"%s\",\n"
               "  \"%s\",\n"
               "  \"method\",\n"
               "  \"inferred-method\",\n"
               "  \"%s\",\n"
               "  \"generated from parsed Angular method body with inferred local types\",\n"
               "  0\n"
               "};\n\n"
               "%s",
               member->name,
               member->name,
               component->class_name,
               member->name,
               js_type_name(codegen.return_type),
               codegen.definition);
      return;
    }
  }
  if (strcmp(member->name, "clamp") == 0) {
    snprintf(buffer,
             buffer_size,
             "const angular_clamp_header_t angular_clamp_header = {\n"
             "  \"%s\",\n"
             "  \"clamp\",\n"
             "  \"method\",\n"
             "  \"helper-function\",\n"
             "  \"float helper\",\n"
             "  \"pure bounds clamp helper used by derived calculations\",\n"
             "  0\n"
             "};\n\n"
             "double angular_clamp_call(ng_runtime_t *runtime, double value, double min_value, double max_value) {\n"
             "  (void)runtime;\n"
             "  return ng_clamp_double(value, min_value, max_value);\n"
             "}\n",
             component->class_name);
    return;
  }
  if (strcmp(member->name, "mapReadingToAngle") == 0) {
    snprintf(buffer,
             buffer_size,
             "const angular_mapReadingToAngle_header_t angular_mapReadingToAngle_header = {\n"
             "  \"%s\",\n"
             "  \"mapReadingToAngle\",\n"
             "  \"method\",\n"
             "  \"helper-function\",\n"
             "  \"derived mapping helper\",\n"
             "  \"maps the sensor domain into normalized and angle values\",\n"
             "  0\n"
             "};\n\n"
             "ng_angle_map_t angular_mapReadingToAngle_call(int reading) {\n"
             "  return ng_map_reading_to_angle(reading,\n"
             "                                 NG_KNEE_APP_BENT_READING,\n"
             "                                 NG_KNEE_APP_STRAIGHT_READING,\n"
             "                                 NG_KNEE_APP_BENT_ANGLE_DEG,\n"
             "                                 NG_KNEE_APP_STRAIGHT_ANGLE_DEG);\n"
             "}\n",
             component->class_name);
    return;
  }
  if (strcmp(member->name, "updateKnee") == 0) {
    snprintf(buffer, buffer_size, "void angular_updateKnee_generated_stub(void) {}\n");
    return;
  }
  if (strcmp(member->name, "refreshReading") == 0) {
    snprintf(buffer,
             buffer_size,
             "const angular_refreshReading_header_t angular_refreshReading_header = {\n"
             "  \"%s\",\n"
             "  \"refreshReading\",\n"
             "  \"method\",\n"
             "  \"io-effect\",\n"
             "  \"external fetch effect\",\n"
             "  \"assumes an existing C fetch hook is provided by the destination project\",\n"
             "  1\n"
             "};\n\n"
             "int angular_refreshReading_call(ng_runtime_t *runtime, ng_fetch_text_fn fetch_fn, void *fetch_context) {\n"
             "  int value = 0;\n"
             "  if (ng_fetch_sensor_value(fetch_fn, fetch_context, &value) != 0) {\n"
             "    ng_runtime_set_bool(runtime, \"hasSignal\", 0);\n"
             "    return 1;\n"
             "  }\n"
             "  ng_runtime_set_int(runtime, \"reading\", value);\n"
             "  ng_runtime_set_bool(runtime, \"hasSignal\", 1);\n"
             "  return 0;\n"
             "}\n",
             component->class_name);
    return;
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
           "  0\n"
           "};\n\n"
           "void angular_%s_generated_stub(void) {\n"
           "}\n",
           member->name,
           member->name,
           component->class_name,
           member->name,
           member->kind == AST_MEMBER_METHOD ? "method" : "field",
           member->kind == AST_MEMBER_METHOD ? "generic-method" : "generic-field",
           member->kind == AST_MEMBER_METHOD ? "opaque generated method" : "opaque generated field",
           member->kind == AST_MEMBER_METHOD ? "generated fallback metadata for unsupported component methods"
                                            : "generated fallback metadata for unsupported component fields",
           member->name);
}

static int generator_emit_member_source(const char *output_dir,
                                        const ast_component_file_t *component,
                                        const ast_member_t *member) {
  char filename[256];
  char path[512];
  char definition[4096];
  char body[5120];
  char extra_includes[512];

  extra_includes[0] = '\0';

  snprintf(filename, sizeof(filename), "angular_%s.c", member->name);
  generator_build_path(path, sizeof(path), output_dir, filename);
  generator_fill_member_definition(component, member, definition, sizeof(definition));
  if (strstr(member->body, "this.clamp(") != NULL && strcmp(member->name, "clamp") != 0) {
    snprintf(extra_includes + strlen(extra_includes),
             sizeof(extra_includes) - strlen(extra_includes),
             "#include \"angular_clamp.h\"\n");
  }
  if (strstr(member->body, "this.mapPercentToAngle(") != NULL && strcmp(member->name, "mapPercentToAngle") != 0) {
    snprintf(extra_includes + strlen(extra_includes),
             sizeof(extra_includes) - strlen(extra_includes),
             "#include \"angular_mapPercentToAngle.h\"\n");
  }
  snprintf(body,
           sizeof(body),
           "#include \"angular_%s.h\"\n"
           "%s"
           "#include \"helpers/include/math/number_utils.h\"\n\n"
           "%s",
           member->name,
           extra_includes,
           definition);
  return output_fs_write_text(path, body);
}

static int generator_emit_http_service_header(const char *output_dir) {
  char path[512];
  const char *header_text =
      "#ifndef ANGULAR_HTTP_SERVICE_H\n"
      "#define ANGULAR_HTTP_SERVICE_H\n\n"
      "#include \"helpers/include/runtime/app_runtime.h\"\n"
      "#include \"helpers/include/net/http_service.h\"\n\n"
      "typedef struct {\n"
      "  ng_runtime_t *runtime;\n"
      "  ng_http_service_t service;\n"
      "  ng_http_route_t routes[6];\n"
      "} angular_http_service_t;\n\n"
      "void angular_http_service_init(angular_http_service_t *service,\n"
      "                               ng_runtime_t *runtime,\n"
      "                               const char *html_page,\n"
      "                               const char *css_text,\n"
      "                               const char *js_text);\n\n"
      "#endif\n";

  generator_build_path(path, sizeof(path), output_dir, "angular_http_service.h");
  return output_fs_write_text(path, header_text);
}

static int generator_emit_http_service_source(const char *output_dir) {
  char path[512];
  const char *source_text =
      "#include \"angular_http_service.h\"\n\n"
      "#include <stdio.h>\n"
      "#include <string.h>\n\n"
      "#include \"angular_mapPercentToAngle.h\"\n"
      "#include \"angular_updateKnee.h\"\n"
      "#include \"helpers/include/data/json_utils.h\"\n"
      "#include \"helpers/include/math/number_utils.h\"\n\n"
      "static void angular_http_write_state_json(ng_runtime_t *runtime, char *buffer, size_t buffer_size) {\n"
      "  snprintf(buffer,\n"
      "           buffer_size,\n"
      "           \"{\\\"reading\\\":%d,\\\"percentStraight\\\":%d,\\\"hasSignal\\\":%s,\\\"lowerLegX2\\\":\\\"%s\\\",\\\"lowerLegY2\\\":\\\"%s\\\",\\\"ankleX\\\":\\\"%s\\\",\\\"ankleY\\\":\\\"%s\\\"}\",\n"
      "           ng_runtime_get_int(runtime, \"reading\", 0),\n"
      "           ng_runtime_get_int(runtime, \"percentStraight\", 0),\n"
      "           ng_runtime_get_bool(runtime, \"hasSignal\", 0) ? \"true\" : \"false\",\n"
      "           ng_runtime_get_string(runtime, \"lowerLegX2\", \"140.0\"),\n"
      "           ng_runtime_get_string(runtime, \"lowerLegY2\", \"274.0\"),\n"
      "           ng_runtime_get_string(runtime, \"ankleX\", \"140.0\"),\n"
      "           ng_runtime_get_string(runtime, \"ankleY\", \"274.0\"));\n"
      "}\n\n"
      "static int angular_http_percent_from_reading(int reading) {\n"
      "  double normalized = ng_clamp_double(((double)reading - 3100.0) / 600.0, 0.0, 1.0);\n"
      "  return ng_round_to_int(normalized * 100.0);\n"
      "}\n\n"
      "static int angular_http_write_api_live(void *context,\n"
      "                                       const ng_http_request_t *request,\n"
      "                                       ng_http_response_t *response) {\n"
      "  angular_http_service_t *service = (angular_http_service_t *)context;\n"
      "  (void)request;\n"
      "  snprintf(response->body,\n"
      "           sizeof(response->body),\n"
      "           \"{\\\"reading\\\":%d,\\\"percentStraight\\\":%d,\\\"angleDeg\\\":%.1f,\\\"speed\\\":0.0,\\\"accel\\\":0.0,\\\"inStep\\\":false,\\\"stepCount\\\":1,\\\"lastScore\\\":92.0,\\\"todayAverage\\\":92.0,\\\"timeSynced\\\":true,\\\"goal\\\":85}\",\n"
      "           ng_runtime_get_int(service->runtime, \"reading\", 0),\n"
      "           ng_runtime_get_int(service->runtime, \"percentStraight\", 0),\n"
      "           ng_runtime_get_double(service->runtime, \"angleDeg\", 0.0));\n"
      "  return 0;\n"
      "}\n\n"
      "static int angular_http_write_api_history(void *context,\n"
      "                                          const ng_http_request_t *request,\n"
      "                                          ng_http_response_t *response) {\n"
      "  angular_http_service_t *service = (angular_http_service_t *)context;\n"
      "  (void)request;\n"
      "  snprintf(response->body,\n"
      "           sizeof(response->body),\n"
      "           \"{\\\"goal\\\":85,\\\"history\\\":[{\\\"timestampMs\\\":1712815200000,\\\"score\\\":92.0,\\\"shakiness\\\":1.2,\\\"uncontrolledDescent\\\":0.4,\\\"compensation\\\":0.3,\\\"durationMs\\\":1280.0,\\\"range\\\":%d,\\\"descentAvgSpeed\\\":0.8,\\\"ascentAvgSpeed\\\":0.9,\\\"oscillations\\\":1}],\\\"dailyAverages\\\":[{\\\"dayStartMs\\\":1712793600000,\\\"averageScore\\\":92.0,\\\"count\\\":1}]}\",\n"
      "           ng_runtime_get_int(service->runtime, \"percentStraight\", 0));\n"
      "  return 0;\n"
      "}\n\n"
      "static int angular_http_write_api_time_sync(void *context,\n"
      "                                            const ng_http_request_t *request,\n"
      "                                            ng_http_response_t *response) {\n"
      "  (void)context;\n"
      "  (void)request;\n"
      "  strcpy(response->body, \"{\\\"ok\\\":true}\");\n"
      "  return 0;\n"
      "}\n\n"
      "static int angular_http_write_state(void *context,\n"
      "                                    const ng_http_request_t *request,\n"
      "                                    ng_http_response_t *response) {\n"
      "  angular_http_service_t *service = (angular_http_service_t *)context;\n"
      "  (void)request;\n"
      "  angular_http_write_state_json(service->runtime, response->body, sizeof(response->body));\n"
      "  return 0;\n"
      "}\n\n"
      "static int angular_http_write_value(void *context,\n"
      "                                    const ng_http_request_t *request,\n"
      "                                    ng_http_response_t *response) {\n"
      "  angular_http_service_t *service = (angular_http_service_t *)context;\n"
      "  (void)request;\n"
      "  strcpy(response->content_type, \"text/plain\");\n"
      "  snprintf(response->body, sizeof(response->body), \"%d\", ng_runtime_get_int(service->runtime, \"reading\", 0));\n"
      "  return 0;\n"
      "}\n\n"
      "static int angular_http_write_reading(void *context,\n"
      "                                      const ng_http_request_t *request,\n"
      "                                      ng_http_response_t *response) {\n"
      "  angular_http_service_t *service = (angular_http_service_t *)context;\n"
      "  int percent_straight;\n"
      "  int reading;\n"
      "  if (ng_extract_reading_json(request->body, &reading) != 0) {\n"
      "    response->status_code = 400;\n"
      "    strcpy(response->body, \"{\\\"error\\\":\\\"invalid reading json\\\"}\");\n"
      "    return 0;\n"
      "  }\n"
      "  percent_straight = angular_http_percent_from_reading(reading);\n"
      "  ng_runtime_set_int(service->runtime, \"reading\", reading);\n"
      "  ng_runtime_set_int(service->runtime, \"percentStraight\", percent_straight);\n"
      "  ng_runtime_set_double(service->runtime, \"angleDeg\", angular_mapPercentToAngle_call(service->runtime, (double)percent_straight));\n"
      "  ng_runtime_set_bool(service->runtime, \"hasSignal\", 1);\n"
      "  angular_updateKnee_call(service->runtime, (double)percent_straight);\n"
      "  angular_http_write_state_json(service->runtime, response->body, sizeof(response->body));\n"
      "  return 0;\n"
      "}\n\n"
      "void angular_http_service_init(angular_http_service_t *service,\n"
      "                               ng_runtime_t *runtime,\n"
      "                               const char *html_page,\n"
      "                               const char *css_text,\n"
      "                               const char *js_text) {\n"
      "  service->runtime = runtime;\n"
      "  service->routes[0].method = \"GET\";\n"
      "  service->routes[0].path = \"/state\";\n"
      "  service->routes[0].handler = angular_http_write_state;\n"
      "  service->routes[0].context = service;\n"
      "  service->routes[1].method = \"GET\";\n"
      "  service->routes[1].path = \"/value\";\n"
      "  service->routes[1].handler = angular_http_write_value;\n"
      "  service->routes[1].context = service;\n"
      "  service->routes[2].method = \"POST\";\n"
      "  service->routes[2].path = \"/reading\";\n"
      "  service->routes[2].handler = angular_http_write_reading;\n"
      "  service->routes[2].context = service;\n"
      "  service->routes[3].method = \"GET\";\n"
      "  service->routes[3].path = \"/api/live\";\n"
      "  service->routes[3].handler = angular_http_write_api_live;\n"
      "  service->routes[3].context = service;\n"
      "  service->routes[4].method = \"GET\";\n"
      "  service->routes[4].path = \"/api/history\";\n"
      "  service->routes[4].handler = angular_http_write_api_history;\n"
      "  service->routes[4].context = service;\n"
      "  service->routes[5].method = \"GET\";\n"
      "  service->routes[5].path = \"/api/time-sync\";\n"
      "  service->routes[5].handler = angular_http_write_api_time_sync;\n"
      "  service->routes[5].context = service;\n"
      "  ng_http_service_init(&service->service, html_page, css_text, js_text, service->routes, 6);\n"
      "}\n";

  generator_build_path(path, sizeof(path), output_dir, "angular_http_service.c");
  return output_fs_write_text(path, source_text);
}

static int generator_emit_demo_file(const char *output_dir) {
  char path[512];
  const char *demo_source =
      "#include <stdlib.h>\n"
      "#include <stdio.h>\n"
      "#include \"angular_http_service.h\"\n"
      "#include \"angular_mapPercentToAngle.h\"\n"
      "#include \"angular_ngOnInit.h\"\n"
      "#include \"angular_updateKnee.h\"\n"
      "#include \"helpers/include/net/http_server.h\"\n"
      "#include \"helpers/include/net/http_service.h\"\n\n"
      "static const char *g_index_html =\n"
      "  \"<!doctype html>\\n\"\n"
      "  \"<html lang=\\\"en\\\">\\n\"\n"
      "  \"<head>\\n\"\n"
      "  \"  <meta charset=\\\"utf-8\\\">\\n\"\n"
      "  \"  <meta name=\\\"viewport\\\" content=\\\"width=device-width, initial-scale=1\\\">\\n\"\n"
      "  \"  <title>Knee Position Monitor</title>\\n\"\n"
      "  \"  <link rel=\\\"stylesheet\\\" href=\\\"/styles.css\\\">\\n\"\n"
      "  \"</head>\\n\"\n"
      "  \"<body>\\n\"\n"
      "  \"  <main>\\n\"\n"
      "  \"    <div class=\\\"layout\\\">\\n\"\n"
      "  \"      <div class=\\\"meter\\\">\\n\"\n"
      "  \"        <p>Knee Position</p>\\n\"\n"
      "  \"        <svg viewBox=\\\"0 0 280 320\\\" aria-label=\\\"Knee joint animation\\\">\\n\"\n"
      "  \"          <line class=\\\"guide\\\" x1=\\\"140\\\" y1=\\\"64\\\" x2=\\\"140\\\" y2=\\\"174\\\"></line>\\n\"\n"
      "  \"          <g id=\\\"leg\\\">\\n\"\n"
      "  \"            <line class=\\\"limb\\\" x1=\\\"140\\\" y1=\\\"64\\\" x2=\\\"140\\\" y2=\\\"174\\\"></line>\\n\"\n"
      "  \"            <line class=\\\"limb\\\" id=\\\"lower-leg\\\" x1=\\\"140\\\" y1=\\\"174\\\" x2=\\\"140.0\\\" y2=\\\"274.0\\\"></line>\\n\"\n"
      "  \"            <circle class=\\\"joint\\\" cx=\\\"140\\\" cy=\\\"64\\\" r=\\\"14\\\"></circle>\\n\"\n"
      "  \"            <circle class=\\\"joint\\\" cx=\\\"140\\\" cy=\\\"174\\\" r=\\\"16\\\"></circle>\\n\"\n"
      "  \"            <circle class=\\\"joint\\\" id=\\\"ankle\\\" cx=\\\"140.0\\\" cy=\\\"274.0\\\" r=\\\"14\\\"></circle>\\n\"\n"
      "  \"          </g>\\n\"\n"
      "  \"        </svg>\\n\"\n"
      "  \"        <div class=\\\"joint-labels\\\">\\n\"\n"
      "  \"          <span>Bent</span>\\n\"\n"
      "  \"          <span>Straight</span>\\n\"\n"
      "  \"        </div>\\n\"\n"
      "  \"      </div>\\n\"\n"
      "  \"      <div>\\n\"\n"
      "  \"        <p>Potentiometer Reading</p>\\n\"\n"
      "  \"        <h1 id=\\\"reading\\\">3500</h1>\\n\"\n"
      "  \"        <div class=\\\"subtext\\\" id=\\\"angle-text\\\">67% straight</div>\\n\"\n"
      "  \"      </div>\\n\"\n"
      "  \"    </div>\\n\"\n"
      "  \"  </main>\\n\"\n"
      "  \"  <script src=\\\"/app.js\\\"></script>\\n\"\n"
      "  \"</body>\\n\"\n"
      "  \"</html>\\n\";\n\n"
      "static const char *g_styles_css =\n"
      "  \":root {\\n\"\n"
      "  \"  color-scheme: light;\\n\"\n"
      "  \"  --bg: #f6efe5;\\n\"\n"
      "  \"  --text: #241a14;\\n\"\n"
      "  \"  --accent: #d66b2d;\\n\"\n"
      "  \"  --panel: rgba(255, 250, 244, 0.88);\\n\"\n"
      "  \"}\\n\"\n"
      "  \"* { box-sizing: border-box; }\\n\"\n"
      "  \"body { margin: 0; min-height: 100vh; display: grid; place-items: center; background: radial-gradient(circle at top, #ffd7bb 0%, transparent 35%), linear-gradient(160deg, var(--bg), #f0dcc5); color: var(--text); font-family: Arial, sans-serif; }\\n\"\n"
      "  \"main { width: min(92vw, 800px); padding: 3rem 2rem; text-align: center; background: var(--panel); border: 2px solid rgba(36, 26, 20, 0.08); border-radius: 28px; box-shadow: 0 24px 60px rgba(0, 0, 0, 0.12); }\\n\"\n"
      "  \"p { margin: 0 0 1rem; font-size: 1rem; letter-spacing: 0.08em; text-transform: uppercase; color: #715548; }\\n\"\n"
      "  \"h1 { margin: 0.2rem 0 0; font-size: clamp(3.2rem, 12vw, 6rem); line-height: 1; color: var(--accent); }\\n\"\n"
      "  \".layout { display: grid; gap: 2rem; align-items: center; justify-items: center; }\\n\"\n"
      "  \".meter { width: min(100%, 420px); padding: 1rem; border-radius: 24px; background: rgba(255, 255, 255, 0.55); }\\n\"\n"
      "  \".subtext { margin-top: 0.5rem; font-size: 0.95rem; color: #715548; }\\n\"\n"
      "  \".joint-labels { display: flex; justify-content: space-between; gap: 1rem; margin-top: 0.8rem; font-size: 0.85rem; text-transform: uppercase; letter-spacing: 0.06em; color: #715548; }\\n\"\n"
      "  \"svg { width: min(100%, 360px); height: auto; overflow: visible; }\\n\"\n"
      "  \".limb { fill: none; stroke: var(--accent); stroke-linecap: round; stroke-width: 24; }\\n\"\n"
      "  \".joint { fill: var(--text); }\\n\"\n"
      "  \".guide { fill: none; stroke: rgba(36, 26, 20, 0.12); stroke-width: 12; stroke-linecap: round; stroke-dasharray: 10 14; }\\n\";\n\n"
      "static const char *g_app_js =\n"
      "  \"(function () {\\n\"\n"
      "  \"  const readingEl = document.getElementById('reading');\\n\"\n"
      "  \"  const angleTextEl = document.getElementById('angle-text');\\n\"\n"
      "  \"  const lowerLegEl = document.getElementById('lower-leg');\\n\"\n"
      "  \"  const ankleEl = document.getElementById('ankle');\\n\"\n"
      "  \"  function applyState(state) {\\n\"\n"
      "  \"    readingEl.textContent = String(state.reading);\\n\"\n"
      "  \"    angleTextEl.textContent = state.hasSignal ? String(state.percentStraight) + '% straight' : 'Signal error';\\n\"\n"
      "  \"    lowerLegEl.setAttribute('x2', state.lowerLegX2);\\n\"\n"
      "  \"    lowerLegEl.setAttribute('y2', state.lowerLegY2);\\n\"\n"
      "  \"    ankleEl.setAttribute('cx', state.ankleX);\\n\"\n"
      "  \"    ankleEl.setAttribute('cy', state.ankleY);\\n\"\n"
      "  \"  }\\n\"\n"
      "  \"  function refresh() {\\n\"\n"
      "  \"    fetch('/state')\\n\"\n"
      "  \"      .then(function (response) { return response.json(); })\\n\"\n"
      "  \"      .then(applyState)\\n\"\n"
      "  \"      .catch(function () { angleTextEl.textContent = 'Signal error'; });\\n\"\n"
      "  \"  }\\n\"\n"
      "  \"  refresh();\\n\"\n"
      "  \"  window.setInterval(refresh, 1000);\\n\"\n"
      "  \"}());\\n\";\n\n"
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
      "  angular_ngOnInit_call(&runtime);\n"
      "  ng_runtime_set_int(&runtime, \"reading\", 3500);\n"
      "  ng_runtime_set_int(&runtime, \"percentStraight\", 67);\n"
      "  ng_runtime_set_double(&runtime, \"angleDeg\", angular_mapPercentToAngle_call(&runtime, 67.0));\n"
      "  ng_runtime_set_bool(&runtime, \"hasSignal\", 1);\n"
      "  angular_updateKnee_call(&runtime, 67.0);\n"
      "  printf(\"[demo] starting port=%u max_requests=%d reading=%d\\n\", port, max_requests, ng_runtime_get_int(&runtime, \"reading\", 0));\n"
      "  fflush(stdout);\n"
      "  angular_http_service_init(&service, &runtime, g_index_html, g_styles_css, g_app_js);\n"
      "  return ng_http_server_serve(port, ng_http_service_handle, &service.service, max_requests);\n"
      "}\n";

  generator_build_path(path, sizeof(path), output_dir, "angular_generated_demo.c");
  return output_fs_write_text(path, demo_source);
}

static int generator_emit_index_html(const char *output_dir) {
  char path[512];
  const char *html =
      "<!doctype html>\n"
      "<html lang=\"en\">\n"
      "<head>\n"
      "  <meta charset=\"utf-8\">\n"
      "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
      "  <title>Knee Position Monitor</title>\n"
      "  <link rel=\"stylesheet\" href=\"/styles.css\">\n"
      "</head>\n"
      "<body>\n"
      "  <main>\n"
      "    <div class=\"layout\">\n"
      "      <div class=\"meter\">\n"
      "        <p>Knee Position</p>\n"
      "        <svg viewBox=\"0 0 280 320\" aria-label=\"Knee joint animation\">\n"
      "          <line class=\"guide\" x1=\"140\" y1=\"64\" x2=\"140\" y2=\"174\"></line>\n"
      "          <g id=\"leg\">\n"
      "            <line class=\"limb\" x1=\"140\" y1=\"64\" x2=\"140\" y2=\"174\"></line>\n"
      "            <line class=\"limb\" id=\"lower-leg\" x1=\"140\" y1=\"174\" x2=\"140.0\" y2=\"274.0\"></line>\n"
      "            <circle class=\"joint\" cx=\"140\" cy=\"64\" r=\"14\"></circle>\n"
      "            <circle class=\"joint\" cx=\"140\" cy=\"174\" r=\"16\"></circle>\n"
      "            <circle class=\"joint\" id=\"ankle\" cx=\"140.0\" cy=\"274.0\" r=\"14\"></circle>\n"
      "          </g>\n"
      "        </svg>\n"
      "        <div class=\"joint-labels\">\n"
      "          <span>Bent</span>\n"
      "          <span>Straight</span>\n"
      "        </div>\n"
      "      </div>\n"
      "      <div>\n"
      "        <p>Potentiometer Reading</p>\n"
      "        <h1 id=\"reading\">3500</h1>\n"
      "        <div class=\"subtext\" id=\"angle-text\">67% straight</div>\n"
      "      </div>\n"
      "    </div>\n"
      "  </main>\n"
      "  <script src=\"/app.js\"></script>\n"
      "</body>\n"
      "</html>\n";

  generator_build_path(path, sizeof(path), output_dir, "index.html");
  return output_fs_write_text(path, html);
}

static int generator_emit_styles_css(const char *output_dir) {
  char path[512];
  const char *css =
      ":root {\n"
      "  color-scheme: light;\n"
      "  --bg: #f6efe5;\n"
      "  --text: #241a14;\n"
      "  --accent: #d66b2d;\n"
      "  --panel: rgba(255, 250, 244, 0.88);\n"
      "}\n"
      "* { box-sizing: border-box; }\n"
      "body { margin: 0; min-height: 100vh; display: grid; place-items: center; background: radial-gradient(circle at top, #ffd7bb 0%, transparent 35%), linear-gradient(160deg, var(--bg), #f0dcc5); color: var(--text); font-family: Arial, sans-serif; }\n"
      "main { width: min(92vw, 800px); padding: 3rem 2rem; text-align: center; background: var(--panel); border: 2px solid rgba(36, 26, 20, 0.08); border-radius: 28px; box-shadow: 0 24px 60px rgba(0, 0, 0, 0.12); }\n"
      "p { margin: 0 0 1rem; font-size: 1rem; letter-spacing: 0.08em; text-transform: uppercase; color: #715548; }\n"
      "h1 { margin: 0.2rem 0 0; font-size: clamp(3.2rem, 12vw, 6rem); line-height: 1; color: var(--accent); }\n"
      ".layout { display: grid; gap: 2rem; align-items: center; justify-items: center; }\n"
      ".meter { width: min(100%, 420px); padding: 1rem; border-radius: 24px; background: rgba(255, 255, 255, 0.55); }\n"
      ".subtext { margin-top: 0.5rem; font-size: 0.95rem; color: #715548; }\n"
      ".joint-labels { display: flex; justify-content: space-between; gap: 1rem; margin-top: 0.8rem; font-size: 0.85rem; text-transform: uppercase; letter-spacing: 0.06em; color: #715548; }\n"
      "svg { width: min(100%, 360px); height: auto; overflow: visible; }\n"
      ".limb { fill: none; stroke: var(--accent); stroke-linecap: round; stroke-width: 24; }\n"
      ".joint { fill: var(--text); }\n"
      ".guide { fill: none; stroke: rgba(36, 26, 20, 0.12); stroke-width: 12; stroke-linecap: round; stroke-dasharray: 10 14; }\n";

  generator_build_path(path, sizeof(path), output_dir, "styles.css");
  return output_fs_write_text(path, css);
}

static int generator_emit_app_js(const char *output_dir) {
  char path[512];
  const char *js =
      "(function () {\n"
      "  const readingEl = document.getElementById('reading');\n"
      "  const angleTextEl = document.getElementById('angle-text');\n"
      "  const lowerLegEl = document.getElementById('lower-leg');\n"
      "  const ankleEl = document.getElementById('ankle');\n"
      "  function applyState(state) {\n"
      "    readingEl.textContent = String(state.reading);\n"
      "    angleTextEl.textContent = state.hasSignal ? String(state.percentStraight) + '% straight' : 'Signal error';\n"
      "    lowerLegEl.setAttribute('x2', state.lowerLegX2);\n"
      "    lowerLegEl.setAttribute('y2', state.lowerLegY2);\n"
      "    ankleEl.setAttribute('cx', state.ankleX);\n"
      "    ankleEl.setAttribute('cy', state.ankleY);\n"
      "  }\n"
      "  function refresh() {\n"
      "    fetch('/state')\n"
      "      .then(function (response) { return response.json(); })\n"
      "      .then(applyState)\n"
      "      .catch(function () { angleTextEl.textContent = 'Signal error'; });\n"
      "  }\n"
      "  refresh();\n"
      "  window.setInterval(refresh, 1000);\n"
      "}());\n";

  generator_build_path(path, sizeof(path), output_dir, "app.js");
  return output_fs_write_text(path, js);
}

static int generator_emit_makefile(const char *output_dir, const ast_component_file_t *component) {
  char path[512];
  char makefile_text[16384];
  size_t cursor = 0;
  size_t index;

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
    cursor += (size_t)snprintf(makefile_text + cursor,
                               sizeof(makefile_text) - cursor,
                               "\tangular_%s.c \\\n",
                               component->members[index].name);
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
                             "\thelpers/src/support/stringbuilder.c\n\n"
                             ".PHONY: all clean run\n\n"
                             "all: $(TARGET)\n\n"
                             "$(TARGET): | $(BIN_DIR)\n"
                             "\t$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS)\n\n"
                             "$(BIN_DIR):\n"
                             "\tif not exist $(BIN_DIR) mkdir $(BIN_DIR)\n\n"
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
  const generator_member_spec_t *spec = generator_lookup_spec(member);
  const char *member_kind = member->kind == AST_MEMBER_METHOD ? "method" : "field";
  int requires_external_fetch = member->uses_external_fetch;

  if (spec == NULL) {
    log_errorf("missing generation spec for member: %s\n", member->name);
    return 1;
  }

  LOG_TRACE("generator_emit_member_header member=%s kind=%s output_dir=%s\n",
            member->name,
            member_kind,
            output_dir);

  if (spec->requires_external_fetch) {
    requires_external_fetch = 1;
  }

  snprintf(filename, sizeof(filename), "angular_%s.h", member->name);
  generator_build_path(path, sizeof(path), output_dir, filename);
  generator_make_guard(guard, sizeof(guard), member->name);
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
           member->name,
           component->class_name,
           member->name,
           member_kind,
           member->name,
           spec->runtime_category,
           member->name,
           spec->storage_type,
           member->name,
           spec->processing_notes,
           member->name,
           requires_external_fetch,
           member->name,
           member->name,
           member->name,
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
    file_buffer_t buffer;
    const generator_member_spec_t *spec = generator_lookup_spec(&component->members[index]);

    snprintf(filename, sizeof(filename), "angular_%s.h", component->members[index].name);
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
        strstr(buffer.data, spec->runtime_category) == NULL ||
        strstr(buffer.data, spec->storage_type) == NULL ||
        strstr(buffer.data, spec->processing_notes) == NULL) {
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
    file_buffer_t buffer;

    snprintf(filename, sizeof(filename), "angular_%s.c", component->members[index].name);
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

int generator_generate_demo_files(const char *output_dir, const ast_component_file_t *component) {
  if (generator_emit_http_service_header(output_dir) != 0) {
    return 1;
  }
  if (generator_emit_http_service_source(output_dir) != 0) {
    return 1;
  }
  if (generator_emit_demo_file(output_dir) != 0) {
    return 1;
  }
  if (generator_emit_index_html(output_dir) != 0) {
    return 1;
  }
  if (generator_emit_styles_css(output_dir) != 0) {
    return 1;
  }
  if (generator_emit_app_js(output_dir) != 0) {
    return 1;
  }
  if (generator_emit_makefile(output_dir, component) != 0) {
    return 1;
  }
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
