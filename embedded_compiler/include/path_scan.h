#ifndef PATH_SCAN_H
#define PATH_SCAN_H

typedef int (*path_scan_callback_t)(const char *path, void *context);

int path_scan_directory(const char *root_path, path_scan_callback_t callback, void *context);

#endif
