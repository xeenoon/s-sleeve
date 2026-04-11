#include "log.h"

#include <stdarg.h>
#include <stdio.h>

static FILE *g_log_file = NULL;

static int log_vprintf_internal(FILE *stream, const char *format, va_list args) {
  int result;
  va_list copy;

  va_copy(copy, args);
  result = vfprintf(stream, format, args);
  if (g_log_file != NULL) {
    vfprintf(g_log_file, format, copy);
    fflush(g_log_file);
  }
  fflush(stream);
  va_end(copy);
  return result;
}

static int log_vtracef_internal(const char *file, int line, const char *format, va_list args) {
  int prefix_result;
  va_list copy;

  prefix_result = fprintf(stdout, "[TRACE %s:%d] ", file, line);
  fflush(stdout);

  if (g_log_file != NULL) {
    fprintf(g_log_file, "[TRACE %s:%d] ", file, line);
    fflush(g_log_file);
  }

  va_copy(copy, args);
  vfprintf(stdout, format, args);
  fflush(stdout);
  if (g_log_file != NULL) {
    vfprintf(g_log_file, format, copy);
    fflush(g_log_file);
  }
  va_end(copy);
  return prefix_result;
}

int log_init(const char *path) {
  g_log_file = fopen(path, "wb");
  return g_log_file == NULL ? 1 : 0;
}

void log_close(void) {
  if (g_log_file != NULL) {
    fclose(g_log_file);
    g_log_file = NULL;
  }
}

int log_printf(const char *format, ...) {
  int result;
  va_list args;

  va_start(args, format);
  result = log_vprintf_internal(stdout, format, args);
  va_end(args);
  return result;
}

int log_errorf(const char *format, ...) {
  int result;
  va_list args;

  va_start(args, format);
  result = log_vprintf_internal(stderr, format, args);
  va_end(args);
  return result;
}

int log_tracef(const char *file, int line, const char *format, ...) {
  int result;
  va_list args;

  va_start(args, format);
  result = log_vtracef_internal(file, line, format, args);
  va_end(args);
  return result;
}

void log_putc(int ch) {
  fputc(ch, stdout);
  fflush(stdout);
  if (g_log_file != NULL) {
    fputc(ch, g_log_file);
    fflush(g_log_file);
  }
}

void log_puts(const char *text) {
  fputs(text, stdout);
  fflush(stdout);
  if (g_log_file != NULL) {
    fputs(text, g_log_file);
    fflush(g_log_file);
  }
}
