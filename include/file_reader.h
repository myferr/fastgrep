#ifndef FILE_READER_H
#define FILE_READER_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

typedef enum {
    READ_SUCCESS,
    READ_ERROR_OPEN,
    READ_ERROR_MMAP,
    READ_ERROR_STAT,
    READ_ERROR_DIRECTORY,
    READ_ERROR_MEMORY,
    READ_ERROR_UNSUPPORTED
} ReadStatus;

typedef struct {
    char* data;
    size_t size;
    int fd;
    int is_mapped;
    char* filepath;
} FileData;

typedef struct {
    FileData** files;
    size_t count;
    size_t capacity;
} FileList;

typedef void (*FileCallback)(const char* filepath, void* userdata);

FileData* file_open(const char* filepath);
void file_close(FileData* file);
ReadStatus file_read(FileData* file);

FileList* filelist_create(void);
void filelist_free(FileList* list);
int filelist_add(FileList* list, FileData* file);
int filelist_add_path(FileList* list, const char* filepath);

int traverse_directory(const char* dirpath, int recursive, FileCallback callback, void* userdata);

size_t count_lines(const char* data, size_t size);
const char* find_line_start(const char* data, size_t size, size_t pos);
const char* find_line_end(const char* data, size_t size, size_t pos);
size_t get_line_number(const char* data, size_t size, size_t pos);

#endif