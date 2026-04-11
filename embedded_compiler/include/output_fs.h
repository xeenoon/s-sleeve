#ifndef OUTPUT_FS_H
#define OUTPUT_FS_H

int output_fs_prepare_clean_directory(const char *path);
int output_fs_create_directory(const char *path);
int output_fs_copy_file(const char *source_path, const char *destination_path);
int output_fs_write_text(const char *path, const char *text);
int output_fs_file_exists(const char *path);

#endif
