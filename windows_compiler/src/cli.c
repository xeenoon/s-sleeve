#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "file_io.h"
#include "ast.h"
#include "component_compose.h"
#include "component_registry.h"
#include "generator.h"
#include "log.h"
#include "parser.h"
#include "path_scan.h"
#include "token.h"
#include "tokenizer.h"

typedef struct {
  size_t file_count;
  size_t token_count;
  size_t validated_file_count;
  char input_dir[MAX_PATH];
  char output_dir[MAX_PATH];
  char runtime_header_path[MAX_PATH];
  char helpers_root_path[MAX_PATH];
  component_registry_t *registry;
} cli_context_t;

static int cli_resolve_absolute_path(char *buffer, size_t buffer_size, const char *path) {
  DWORD result;

  if (buffer == NULL || buffer_size == 0 || path == NULL || path[0] == '\0') {
    return 1;
  }

  result = GetFullPathNameA(path, (DWORD)buffer_size, buffer, NULL);
  if (result == 0 || result >= buffer_size) {
    return 1;
  }

  return 0;
}

static int validate_ast(const char *path, const ast_file_t *ast) {
  if (strstr(path, ".component.ng") != NULL) {
    return ast->kind == PARSER_FILE_COMPONENT &&
           ast->data.component.has_component_decorator &&
           ast->data.component.class_name[0] != '\0' &&
           ast->data.component.selector[0] != '\0';
  }

  if (strstr(path, ".component.html") != NULL) {
    return ast->kind == PARSER_FILE_TEMPLATE &&
           ast->data.template_file.element_count > 0;
  }

  if (strstr(path, ".component.css") != NULL) {
    return ast->kind == PARSER_FILE_STYLE &&
           ast->data.style.has_balanced_blocks &&
           ast->data.style.rule_count > 0;
  }

  return ast->kind != PARSER_FILE_UNKNOWN;
}

static int tokenize_file(const char *path, void *context) {
  cli_context_t *cli_context = (cli_context_t *)context;
  file_buffer_t buffer;
  tokenizer_t tokenizer;
  parse_summary_t summary;
  ast_file_t ast;
  int is_valid;
  component_unit_t *unit;

  if (!component_registry_is_component_path(path)) {
    LOG_TRACE("tokenize_file skip non-ast asset path=%s\n", path);
    return 0;
  }

  if (file_read_all(path, &buffer) != 0) {
    log_errorf("failed to read file: %s\n", path);
    return 1;
  }

  LOG_TRACE("tokenize_file start path=%s bytes=%zu\n", path, buffer.size);
  log_printf("== FILE %s ==\n", path);
  tokenizer_init(&tokenizer, buffer.data, buffer.size);

  for (;;) {
    token_t token = tokenizer_next(&tokenizer);
    token_print(&token);
    cli_context->token_count += 1;
    if (token.kind == TOKEN_EOF) {
      break;
    }
  }

  if (parser_summarize_file(path, buffer.data, buffer.size, &summary) != 0) {
    LOG_TRACE("tokenize_file parser_summarize_file failed path=%s\n", path);
    file_buffer_free(&buffer);
    return 1;
  }

  if (parser_parse_file(path, buffer.data, buffer.size, &ast) != 0) {
    LOG_TRACE("tokenize_file parser_parse_file failed path=%s\n", path);
    file_buffer_free(&buffer);
    return 1;
  }

  parser_print_summary(&summary);
  ast_file_print(&ast);
  is_valid = validate_ast(path, &ast);
  log_printf("VALIDATION %s\n\n", is_valid ? "OK" : "FAILED");

  if (!is_valid) {
    LOG_TRACE("tokenize_file validation failed path=%s\n", path);
    file_buffer_free(&buffer);
    return 1;
  }

  unit = component_registry_get_or_create(cli_context->registry, path);
  if (unit == NULL) {
    log_errorf("failed to register component file: %s\n", path);
    file_buffer_free(&buffer);
    return 1;
  }

  if (ast.kind == PARSER_FILE_COMPONENT) {
    LOG_TRACE("tokenize_file capture component path=%s bytes=%zu\n", path, buffer.size);
    unit->component_ast = ast.data.component;
    unit->has_component_ast = 1;
    strcpy(unit->selector, ast.data.component.selector);
    strncpy(unit->ng_path, path, sizeof(unit->ng_path) - 1);
    memset(&buffer, 0, sizeof(buffer));
  } else if (strstr(path, ".component.html") != NULL) {
    LOG_TRACE("tokenize_file capture html path=%s bytes=%zu\n", path, buffer.size);
    file_buffer_free(&unit->html_buffer);
    unit->html_buffer = buffer;
    unit->has_html = 1;
    strncpy(unit->html_path, path, sizeof(unit->html_path) - 1);
    memset(&buffer, 0, sizeof(buffer));
  } else if (strstr(path, ".component.css") != NULL) {
    LOG_TRACE("tokenize_file capture css path=%s bytes=%zu\n", path, buffer.size);
    file_buffer_free(&unit->css_buffer);
    unit->css_buffer = buffer;
    unit->has_css = 1;
    strncpy(unit->css_path, path, sizeof(unit->css_path) - 1);
    memset(&buffer, 0, sizeof(buffer));
  }

  LOG_TRACE("tokenize_file success path=%s\n", path);
  log_printf("\n");

  cli_context->file_count += 1;
  cli_context->validated_file_count += 1;
  file_buffer_free(&buffer);
  return 0;
}

static int cli_generate_outputs(cli_context_t *context) {
  char composed_html[65536];
  char composed_css[65536];
  const component_unit_t *root;

  if (component_registry_validate(context->registry) != 0) {
    log_errorf("component registry validation failed\n");
    return 1;
  }

  root = component_registry_root(context->registry);
  if (root == NULL) {
    log_errorf("missing root component\n");
    return 1;
  }

  if (component_compose_html(context->registry, root, composed_html, sizeof(composed_html)) != 0) {
    log_errorf("failed to compose component html\n");
    return 1;
  }

  if (component_compose_css(context->registry, root, composed_css, sizeof(composed_css)) != 0) {
    log_errorf("failed to compose component css\n");
    return 1;
  }

  LOG_TRACE("cli_generate_outputs output_dir=%s component_class=%s html_bytes=%zu css_bytes=%zu components=%zu\n",
            context->output_dir,
            root->component_ast.class_name,
            strlen(composed_html),
            strlen(composed_css),
            context->registry->component_count);

  if (generator_copy_runtime_header(context->runtime_header_path, context->output_dir) != 0) {
    log_errorf("failed to copy runtime header into %s\n", context->output_dir);
    return 1;
  }

  if (generator_generate_component_headers(context->output_dir, &root->component_ast) != 0) {
    log_errorf("failed to generate component headers into %s\n", context->output_dir);
    return 1;
  }

  if (generator_validate_component_headers(context->output_dir, &root->component_ast) != 0) {
    return 1;
  }

  if (generator_generate_component_sources(context->output_dir, &root->component_ast) != 0) {
    log_errorf("failed to generate component sources into %s\n", context->output_dir);
    return 1;
  }

  if (generator_validate_component_sources(context->output_dir, &root->component_ast) != 0) {
    return 1;
  }

  if (generator_generate_demo_files(context->output_dir,
                                    context->input_dir,
                                    &root->component_ast,
                                    composed_html,
                                    composed_css) != 0) {
    log_errorf("failed to generate demo files into %s\n", context->output_dir);
    return 1;
  }

  if (generator_validate_demo_files(context->output_dir) != 0) {
    return 1;
  }

  if (generator_copy_helpers(context->helpers_root_path, context->output_dir) != 0) {
    log_errorf("failed to copy helpers into %s\n", context->output_dir);
    return 1;
  }

  if (generator_validate_helpers(context->output_dir) != 0) {
    return 1;
  }

  return 0;
}

static void cli_free_context_buffers(cli_context_t *context) {
  if (context->registry != NULL) {
    component_registry_free(context->registry);
    free(context->registry);
    context->registry = NULL;
  }
}

int cli_run(int argc, char **argv) {
  cli_context_t context = {0};
  const char *input_dir;

  if (cli_resolve_absolute_path(context.output_dir, sizeof(context.output_dir), "angular_test") != 0 ||
      cli_resolve_absolute_path(context.runtime_header_path, sizeof(context.runtime_header_path), "angular_runtime.h") != 0 ||
      cli_resolve_absolute_path(context.helpers_root_path, sizeof(context.helpers_root_path), "helpers") != 0) {
    fprintf(stderr, "failed to resolve compiler paths\n");
    return 1;
  }
  context.registry = (component_registry_t *)malloc(sizeof(*context.registry));
  if (context.registry == NULL) {
    fprintf(stderr, "failed to allocate component registry\n");
    return 1;
  }
  component_registry_init(context.registry);

  if (log_init("log.txt") != 0) {
    fprintf(stderr, "failed to open log.txt\n");
    return 1;
  }

  LOG_TRACE("cli_run argc=%d\n", argc);

  if (argc != 2 && argc != 3) {
    log_errorf("usage: %s <input-folder> [output-folder]\n", argv[0]);
    log_close();
    return 1;
  }

  input_dir = argv[1];
  if (cli_resolve_absolute_path(context.input_dir, sizeof(context.input_dir), input_dir) != 0) {
    log_errorf("failed to resolve input directory: %s\n", input_dir);
    cli_free_context_buffers(&context);
    log_close();
    return 1;
  }
  if (argc == 3) {
    if (cli_resolve_absolute_path(context.output_dir, sizeof(context.output_dir), argv[2]) != 0) {
      log_errorf("failed to resolve output directory: %s\n", argv[2]);
      cli_free_context_buffers(&context);
      log_close();
      return 1;
    }
  }

  LOG_TRACE("cli_run input_dir=%s output_dir=%s runtime_header_path=%s\n",
            input_dir,
            context.output_dir,
            context.runtime_header_path);

  if (generator_prepare_output_directory(context.output_dir) != 0) {
    log_errorf("failed to prepare output directory: %s\n", context.output_dir);
    log_close();
    return 1;
  }

  if (path_scan_directory(input_dir, tokenize_file, &context) != 0) {
    LOG_TRACE("cli_run path_scan_directory failed input_dir=%s\n", input_dir);
    cli_free_context_buffers(&context);
    log_close();
    return 1;
  }

  if (cli_generate_outputs(&context) != 0) {
    LOG_TRACE("cli_run cli_generate_outputs failed output_dir=%s\n", context.output_dir);
    cli_free_context_buffers(&context);
    log_close();
    return 1;
  }

  LOG_TRACE("cli_run finished scan files=%zu tokens=%zu validated=%zu\n",
            context.file_count,
            context.token_count,
            context.validated_file_count);
  log_printf("Processed %zu files, %zu tokens, %zu validated summaries.\n",
             context.file_count,
             context.token_count,
             context.validated_file_count);
  log_printf("Generated headers in %s\n", context.output_dir);
  cli_free_context_buffers(&context);
  log_close();
  return 0;
}
