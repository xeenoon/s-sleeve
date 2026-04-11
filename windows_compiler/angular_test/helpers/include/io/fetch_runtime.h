#ifndef NG_FETCH_RUNTIME_H
#define NG_FETCH_RUNTIME_H

#include <stddef.h>

typedef int (*ng_fetch_text_fn)(void *context,
                                const char *path,
                                char *buffer,
                                size_t buffer_size,
                                size_t *out_length);

int ng_parse_sensor_text(const char *text, int *out_value);
int ng_refresh_sensor_value(ng_fetch_text_fn fetch_fn,
                            void *context,
                            const char *path,
                            int *out_value);

#endif
