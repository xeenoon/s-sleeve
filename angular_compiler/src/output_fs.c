#include "output_fs.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "log.h"

static int output_fs_remove_tree_internal(const char *path) {
  char search_path[1024];
  WIN32_FIND_DATAA find_data;
  HANDLE handle;

  LOG_TRACE("output_fs_remove_tree_internal enter path=%s\n", path);

  if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) {
    LOG_TRACE("output_fs_remove_tree_internal path missing=%s\n", path);
    return 0;
  }

  if (strlen(path) + 3 >= sizeof(search_path)) {
    return 1;
  }

  snprintf(search_path, sizeof(search_path), "%s\\*", path);
  handle = FindFirstFileA(search_path, &find_data);
  if (handle != INVALID_HANDLE_VALUE) {
    do {
      char child_path[1024];

      if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
        continue;
      }

      if (strlen(path) + strlen(find_data.cFileName) + 2 >= sizeof(child_path)) {
        FindClose(handle);
        return 1;
      }

      snprintf(child_path, sizeof(child_path), "%s\\%s", path, find_data.cFileName);

      if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        LOG_TRACE("output_fs_remove_tree_internal recurse dir=%s\n", child_path);
        if (output_fs_remove_tree_internal(child_path) != 0) {
          FindClose(handle);
          return 1;
        }
      } else {
        LOG_TRACE("output_fs_remove_tree_internal delete file=%s\n", child_path);
        if (!DeleteFileA(child_path)) {
          log_errorf("DeleteFileA failed path=%s error=%lu\n",
                     child_path,
                     (unsigned long)GetLastError());
          FindClose(handle);
          return 1;
        }
      }
    } while (FindNextFileA(handle, &find_data) != 0);

    FindClose(handle);
  }

  if (!RemoveDirectoryA(path)) {
    log_errorf("RemoveDirectoryA failed path=%s error=%lu\n",
               path,
               (unsigned long)GetLastError());
    return 1;
  }

  LOG_TRACE("output_fs_remove_tree_internal removed path=%s\n", path);
  return 0;
}

int output_fs_prepare_clean_directory(const char *path) {
  LOG_TRACE("output_fs_prepare_clean_directory start path=%s\n", path);
  if (output_fs_remove_tree_internal(path) != 0) {
    LOG_TRACE("output_fs_prepare_clean_directory remove failed path=%s\n", path);
    return 1;
  }

  if (!CreateDirectoryA(path, NULL)) {
    DWORD last_error = GetLastError();
    LOG_TRACE("output_fs_prepare_clean_directory CreateDirectoryA failed path=%s error=%lu\n",
              path,
              (unsigned long)last_error);
    if (last_error != ERROR_ALREADY_EXISTS) {
      return 1;
    }
  }

  LOG_TRACE("output_fs_prepare_clean_directory ready path=%s\n", path);
  return 0;
}

int output_fs_create_directory(const char *path) {
  LOG_TRACE("output_fs_create_directory path=%s\n", path);
  if (!CreateDirectoryA(path, NULL)) {
    DWORD last_error = GetLastError();
    if (last_error != ERROR_ALREADY_EXISTS) {
      log_errorf("CreateDirectoryA failed path=%s error=%lu\n",
                 path,
                 (unsigned long)last_error);
      return 1;
    }
  }

  return 0;
}

int output_fs_copy_file(const char *source_path, const char *destination_path) {
  LOG_TRACE("output_fs_copy_file source=%s destination=%s\n", source_path, destination_path);
  if (!CopyFileA(source_path, destination_path, FALSE)) {
    log_errorf("CopyFileA failed source=%s destination=%s error=%lu\n",
               source_path,
               destination_path,
               (unsigned long)GetLastError());
    return 1;
  }

  LOG_TRACE("output_fs_copy_file success destination=%s\n", destination_path);
  return 0;
}

int output_fs_write_text(const char *path, const char *text) {
  FILE *file = fopen(path, "wb");
  size_t length;

  LOG_TRACE("output_fs_write_text start path=%s\n", path);

  if (file == NULL) {
    log_errorf("fopen failed for write path=%s\n", path);
    return 1;
  }

  length = strlen(text);
  LOG_TRACE("output_fs_write_text bytes=%zu path=%s\n", length, path);
  if (fwrite(text, 1, length, file) != length) {
    log_errorf("fwrite failed path=%s\n", path);
    fclose(file);
    return 1;
  }

  fclose(file);
  LOG_TRACE("output_fs_write_text success path=%s\n", path);
  return 0;
}

int output_fs_file_exists(const char *path) {
  int exists = GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
  LOG_TRACE("output_fs_file_exists path=%s exists=%d\n", path, exists);
  return exists;
}
