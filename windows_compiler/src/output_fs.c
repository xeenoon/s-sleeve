#include "output_fs.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <tlhelp32.h>

#include "log.h"

static void output_fs_normalize_path(char *path) {
  size_t index;
  for (index = 0; path[index] != '\0'; ++index) {
    if (path[index] == '/') {
      path[index] = '\\';
    } else if (path[index] >= 'A' && path[index] <= 'Z') {
      path[index] = (char)(path[index] - 'A' + 'a');
    }
  }
}

static int output_fs_kill_process_holding_file(const char *path) {
  HANDLE snapshot;
  PROCESSENTRY32 process_entry;
  char target_path[MAX_PATH];
  DWORD resolved_length;

  resolved_length = GetFullPathNameA(path, (DWORD)sizeof(target_path), target_path, NULL);
  if (resolved_length == 0 || resolved_length >= sizeof(target_path)) {
    LOG_TRACE("output_fs_kill_process_holding_file failed resolve path=%s error=%lu\n",
              path,
              (unsigned long)GetLastError());
    return 1;
  }

  output_fs_normalize_path(target_path);
  snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    log_errorf("CreateToolhelp32Snapshot failed error=%lu\n", (unsigned long)GetLastError());
    return 1;
  }

  memset(&process_entry, 0, sizeof(process_entry));
  process_entry.dwSize = sizeof(process_entry);
  if (!Process32First(snapshot, &process_entry)) {
    CloseHandle(snapshot);
    log_errorf("Process32First failed error=%lu\n", (unsigned long)GetLastError());
    return 1;
  }

  do {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE,
                                 FALSE,
                                 process_entry.th32ProcessID);
    if (process != NULL) {
      char process_path[MAX_PATH];
      DWORD process_path_size = (DWORD)sizeof(process_path);
      if (QueryFullProcessImageNameA(process, 0, process_path, &process_path_size)) {
        output_fs_normalize_path(process_path);
        if (strcmp(process_path, target_path) == 0) {
          LOG_TRACE("output_fs_kill_process_holding_file terminating pid=%lu path=%s\n",
                    (unsigned long)process_entry.th32ProcessID,
                    process_path);
          if (!TerminateProcess(process, 0)) {
            log_errorf("TerminateProcess failed pid=%lu path=%s error=%lu\n",
                       (unsigned long)process_entry.th32ProcessID,
                       process_path,
                       (unsigned long)GetLastError());
            CloseHandle(process);
            CloseHandle(snapshot);
            return 1;
          }
          WaitForSingleObject(process, 5000);
          CloseHandle(process);
          CloseHandle(snapshot);
          return 0;
        }
      }
      CloseHandle(process);
    }
  } while (Process32Next(snapshot, &process_entry));

  CloseHandle(snapshot);
  LOG_TRACE("output_fs_kill_process_holding_file no matching process path=%s\n", target_path);
  return 1;
}

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
        int delete_recovered = 0;
        LOG_TRACE("output_fs_remove_tree_internal delete file=%s\n", child_path);
        if (!DeleteFileA(child_path)) {
          DWORD last_error = GetLastError();
          int attempt;
          LOG_TRACE("output_fs_remove_tree_internal delete failed path=%s error=%lu\n",
                    child_path,
                    (unsigned long)last_error);
          if (last_error == ERROR_ACCESS_DENIED || last_error == ERROR_SHARING_VIOLATION) {
            if (output_fs_kill_process_holding_file(child_path) == 0) {
              for (attempt = 0; attempt < 20; ++attempt) {
                Sleep(100);
                if (DeleteFileA(child_path)) {
                  delete_recovered = 1;
                  LOG_TRACE("output_fs_remove_tree_internal delete recovered path=%s attempt=%d\n",
                            child_path,
                            attempt + 1);
                  break;
                }
              }
              last_error = GetLastError();
            }
          }
          if (!delete_recovered) {
            log_errorf("DeleteFileA failed path=%s error=%lu\n",
                       child_path,
                       (unsigned long)last_error);
            FindClose(handle);
            return 1;
          }
        }
      }
    } while (FindNextFileA(handle, &find_data) != 0);

    FindClose(handle);
  }

  if (!RemoveDirectoryA(path)) {
    DWORD last_error = GetLastError();
    int attempt;
    if (last_error == ERROR_ACCESS_DENIED || last_error == ERROR_SHARING_VIOLATION || last_error == ERROR_DIR_NOT_EMPTY) {
      for (attempt = 0; attempt < 20; ++attempt) {
        Sleep(100);
        if (RemoveDirectoryA(path)) {
          LOG_TRACE("output_fs_remove_tree_internal directory remove recovered path=%s attempt=%d\n",
                    path,
                    attempt + 1);
          LOG_TRACE("output_fs_remove_tree_internal removed path=%s\n", path);
          return 0;
        }
        last_error = GetLastError();
      }
    }
    log_errorf("RemoveDirectoryA failed path=%s error=%lu\n",
               path,
               (unsigned long)last_error);
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
