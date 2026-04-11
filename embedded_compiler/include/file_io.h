#ifndef FILE_IO_H
#define FILE_IO_H

#include <stddef.h>

typedef struct {
  char *data;
  size_t size;
} file_buffer_t;

int file_read_all(const char *path, file_buffer_t *out_buffer);
void file_buffer_free(file_buffer_t *buffer);

#endif
