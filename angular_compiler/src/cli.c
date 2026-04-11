#include "cli.h"

#include <stdio.h>
#include <string.h>

#include "file_io.h"
#include "ast.h"
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
  const char *output_dir;
  const char *runtime_header_path;
  const char *helpers_root_path;
} cli_context_t;

static int validate_ast(const char *path, const ast_file_t *ast) {
  if (strstr(path, "app.component.ng") != NULL) {
    return ast->kind == PARSER_FILE_COMPONENT &&
           ast->data.component.has_component_decorator &&
           strcmp(ast->data.component.class_name, "AppComponent") == 0 &&
           ast->data.component.member_count > 0;
  }

  if (strstr(path, "app.component.html") != NULL) {
    return ast->kind == PARSER_FILE_TEMPLATE &&
           ast->data.template_file.interpolation_count > 0 &&
           ast->data.template_file.attribute_binding_count > 0 &&
           ast->data.template_file.element_count > 0;
  }

  if (strstr(path, "app.component.css") != NULL) {
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

  if (ast.kind == PARSER_FILE_COMPONENT) {
    LOG_TRACE("tokenize_file entering generation path=%s output_dir=%s\n", path, cli_context->output_dir);
    if (generator_copy_runtime_header(cli_context->runtime_header_path, cli_context->output_dir) != 0) {
      file_buffer_free(&buffer);
      log_errorf("failed to copy runtime header into %s\n", cli_context->output_dir);
      return 1;
    }

    if (generator_generate_component_headers(cli_context->output_dir, &ast.data.component) != 0) {
      file_buffer_free(&buffer);
      log_errorf("failed to generate component headers into %s\n", cli_context->output_dir);
      return 1;
    }

    if (generator_validate_component_headers(cli_context->output_dir, &ast.data.component) != 0) {
      file_buffer_free(&buffer);
      return 1;
    }

    if (generator_generate_component_sources(cli_context->output_dir, &ast.data.component) != 0) {
      file_buffer_free(&buffer);
      log_errorf("failed to generate component sources into %s\n", cli_context->output_dir);
      return 1;
    }

    if (generator_validate_component_sources(cli_context->output_dir, &ast.data.component) != 0) {
      file_buffer_free(&buffer);
      return 1;
    }

    if (generator_generate_demo_files(cli_context->output_dir, &ast.data.component) != 0) {
      file_buffer_free(&buffer);
      log_errorf("failed to generate demo files into %s\n", cli_context->output_dir);
      return 1;
    }

    if (generator_validate_demo_files(cli_context->output_dir) != 0) {
      file_buffer_free(&buffer);
      return 1;
    }

    if (generator_copy_helpers(cli_context->helpers_root_path, cli_context->output_dir) != 0) {
      file_buffer_free(&buffer);
      log_errorf("failed to copy helpers into %s\n", cli_context->output_dir);
      return 1;
    }

    if (generator_validate_helpers(cli_context->output_dir) != 0) {
      file_buffer_free(&buffer);
      return 1;
    }
  }

  LOG_TRACE("tokenize_file success path=%s\n", path);
  log_printf("\n");

  cli_context->file_count += 1;
  cli_context->validated_file_count += 1;
  file_buffer_free(&buffer);
  return 0;
}

int cli_run(int argc, char **argv) {
  cli_context_t context = {0, 0, 0, "angular_test", "angular_runtime.h", "helpers"};
  const char *input_dir;

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
  if (argc == 3) {
    context.output_dir = argv[2];
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
  log_close();
  return 0;
}
