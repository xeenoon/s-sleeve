#ifndef LOG_H
#define LOG_H

#define LOG_TRACE_ENABLED 1

int log_init(const char *path);
void log_close(void);
int log_printf(const char *format, ...);
int log_errorf(const char *format, ...);
int log_tracef(const char *file, int line, const char *format, ...);
void log_putc(int ch);
void log_puts(const char *text);

#if LOG_TRACE_ENABLED
#define LOG_TRACE(...) log_tracef(__FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG_TRACE(...) ((void)0)
#endif

#endif
