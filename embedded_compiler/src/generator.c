#include "generator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asset_writer.h"
#include "file_io.h"
#include "log.h"
#include "output_fs.h"
#include "path_scan.h"

#define GENERATOR_MAX_ROUTES 32

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
                                       const char *html_source,
                                       const char *css_source) {
  char app_js_path[512];
  file_buffer_t js_buffer;
  generator_route_collection_t routes;

  memset(&js_buffer, 0, sizeof(js_buffer));
  generator_build_path(app_js_path, sizeof(app_js_path), input_dir, "app.js");

  if (file_read_all(app_js_path, &js_buffer) != 0) {
    log_errorf("failed to read embedded app.js source: %s\n", app_js_path);
    return 1;
  }

  if (generator_collect_route_assets(input_dir, &routes) != 0) {
    file_buffer_free(&js_buffer);
    return 1;
  }

  if (generator_emit_runtime_header(output_dir) != 0 ||
      generator_emit_runtime_source(output_dir, &routes) != 0 ||
      generator_emit_cpp_bundle(output_dir, html_source, css_source, js_buffer.data) != 0 ||
      generator_write_text_asset(output_dir, "index.html", html_source) != 0 ||
      generator_write_text_asset(output_dir, "styles.css", css_source) != 0 ||
      generator_write_text_asset(output_dir, "app.js", js_buffer.data) != 0) {
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
