#ifndef GENERATOR_H
#define GENERATOR_H

#include "ast.h"

int generator_prepare_output_directory(const char *output_dir);
int generator_copy_runtime_header(const char *runtime_header_path, const char *output_dir);
int generator_generate_component_headers(const char *output_dir, const ast_component_file_t *component);
int generator_validate_component_headers(const char *output_dir, const ast_component_file_t *component);
int generator_generate_component_sources(const char *output_dir, const ast_component_file_t *component);
int generator_validate_component_sources(const char *output_dir, const ast_component_file_t *component);
int generator_generate_demo_files(const char *output_dir,
                                  const ast_component_file_t *component,
                                  const char *html_source,
                                  const char *css_source);
int generator_validate_demo_files(const char *output_dir);
int generator_copy_helpers(const char *helpers_root_path, const char *output_dir);
int generator_validate_helpers(const char *output_dir);

#endif
