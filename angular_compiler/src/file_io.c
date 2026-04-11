#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>

#include "log.h"

int file_read_all(const char *path, file_buffer_t *out_buffer) {
  FILE *file = fopen(path, "rb");
  long file_size;
  char *buffer;

  LOG_TRACE("file_read_all start path=%s\n", path);

  if (file == NULL) {
    LOG_TRACE("file_read_all fopen failed path=%s\n", path);
    return 1;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    LOG_TRACE("file_read_all seek_end failed path=%s\n", path);
    fclose(file);
    return 1;
  }

  file_size = ftell(file);
  if (file_size < 0) {
    LOG_TRACE("file_read_all ftell failed path=%s\n", path);
    fclose(file);
    return 1;
  }

  LOG_TRACE("file_read_all size=%ld path=%s\n", file_size, path);

  if (fseek(file, 0, SEEK_SET) != 0) {
    LOG_TRACE("file_read_all seek_set failed path=%s\n", path);
    fclose(file);
    return 1;
  }

  buffer = (char *)malloc((size_t)file_size + 1);
  if (buffer == NULL) {
    LOG_TRACE("file_read_all malloc failed bytes=%zu path=%s\n", (size_t)file_size + 1, path);
    fclose(file);
    return 1;
  }

  if (fread(buffer, 1, (size_t)file_size, file) != (size_t)file_size) {
    LOG_TRACE("file_read_all fread failed path=%s\n", path);
    free(buffer);
    fclose(file);
    return 1;
  }

  buffer[file_size] = '\0';
  fclose(file);

  out_buffer->data = buffer;
  out_buffer->size = (size_t)file_size;
  LOG_TRACE("file_read_all success bytes=%zu path=%s\n", out_buffer->size, path);
  return 0;
}

void file_buffer_free(file_buffer_t *buffer) {
  if (buffer == NULL) {
    return;
  }

  LOG_TRACE("file_buffer_free size=%zu\n", buffer->size);
  free(buffer->data);
  buffer->data = NULL;
  buffer->size = 0;
}
