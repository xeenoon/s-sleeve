#ifndef GENERATOR_H
#define GENERATOR_H

#include "ast.h"

int generator_prepare_output_directory(const char *output_dir);
int generator_generate_embedded_bundle(const char *output_dir,
                                       const char *input_dir,
                                       const ast_component_file_t *component,
                                       const char *html_source,
                                       const char *css_source);
int generator_validate_embedded_bundle(const char *output_dir);

#endif
