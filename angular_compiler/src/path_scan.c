#include "path_scan.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "log.h"

static int has_dot_directory_name(const char *name) {
  return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

static int has_supported_extension(const char *path) {
  const char *last_dot = strrchr(path, '.');

  if (last_dot == NULL) {
    return 0;
  }

  return strcmp(last_dot, ".ng") == 0 ||
         strcmp(last_dot, ".html") == 0 ||
         strcmp(last_dot, ".css") == 0 ||
         strcmp(last_dot, ".ts") == 0;
}

static int path_scan_directory_internal(const char *root_path,
                                        path_scan_callback_t callback,
                                        void *context) {
  char search_path[MAX_PATH];
  WIN32_FIND_DATAA find_data;
  HANDLE handle;

  LOG_TRACE("path_scan_directory_internal enter root=%s\n", root_path);

  if (snprintf(search_path, sizeof(search_path), "%s\\*", root_path) < 0) {
    LOG_TRACE("path_scan_directory_internal failed search path format root=%s\n", root_path);
    return 1;
  }

  handle = FindFirstFileA(search_path, &find_data);
  if (handle == INVALID_HANDLE_VALUE) {
    log_errorf("failed to scan directory: %s\n", root_path);
    return 1;
  }

  do {
    char child_path[MAX_PATH];

    if (has_dot_directory_name(find_data.cFileName)) {
      LOG_TRACE("path_scan skip dot entry root=%s name=%s\n", root_path, find_data.cFileName);
      continue;
    }

    if (snprintf(child_path, sizeof(child_path), "%s\\%s", root_path, find_data.cFileName) < 0) {
      LOG_TRACE("path_scan failed child path format root=%s name=%s\n", root_path, find_data.cFileName);
      FindClose(handle);
      return 1;
    }

    LOG_TRACE("path_scan discovered path=%s attributes=%lu\n",
              child_path,
              (unsigned long)find_data.dwFileAttributes);

    if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
      LOG_TRACE("path_scan descending directory=%s\n", child_path);
      if (path_scan_directory_internal(child_path, callback, context) != 0) {
        FindClose(handle);
        return 1;
      }
      continue;
    }

    if (has_supported_extension(child_path)) {
      LOG_TRACE("path_scan invoking callback path=%s\n", child_path);
      if (callback(child_path, context) != 0) {
        LOG_TRACE("path_scan callback failed path=%s\n", child_path);
        FindClose(handle);
        return 1;
      }
    } else {
      LOG_TRACE("path_scan ignored unsupported file=%s\n", child_path);
    }
  } while (FindNextFileA(handle, &find_data) != 0);

  FindClose(handle);
  LOG_TRACE("path_scan_directory_internal leave root=%s\n", root_path);
  return 0;
}

int path_scan_directory(const char *root_path, path_scan_callback_t callback, void *context) {
  LOG_TRACE("path_scan_directory start root=%s\n", root_path);
  return path_scan_directory_internal(root_path, callback, context);
}
