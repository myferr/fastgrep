#include "../include/file_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#define SMALL_FILE_THRESHOLD (1 * 1024 * 1024)
#define INITIAL_FILE_CAPACITY 1024

FileData* file_open(const char* filepath) {
    FileData* file = (FileData*)malloc(sizeof(FileData));
    if (!file) return NULL;

    file->data = NULL;
    file->size = 0;
    file->fd = -1;
    file->is_mapped = 0;
    file->filepath = strdup(filepath);

    return file;
}

void file_close(FileData* file) {
    if (!file) return;

    if (file->data) {
        if (file->is_mapped) {
            if (file->fd >= 0) {
                munmap(file->data, file->size);
            }
        } else {
            free(file->data);
        }
    }

    if (file->fd >= 0) {
        close(file->fd);
    }

    if (file->filepath) {
        free(file->filepath);
    }

    free(file);
}

ReadStatus file_read(FileData* file) {
    if (!file || !file->filepath) {
        return READ_ERROR_OPEN;
    }

    file->fd = open(file->filepath, O_RDONLY);
    if (file->fd < 0) {
        return READ_ERROR_OPEN;
    }

    struct stat st;
    if (fstat(file->fd, &st) != 0) {
        close(file->fd);
        file->fd = -1;
        return READ_ERROR_STAT;
    }

    if (S_ISDIR(st.st_mode)) {
        close(file->fd);
        file->fd = -1;
        return READ_ERROR_DIRECTORY;
    }

    file->size = st.st_size;

    if (file->size > SMALL_FILE_THRESHOLD) {
        file->data = (char*)mmap(NULL, file->size, PROT_READ, MAP_PRIVATE, file->fd, 0);
        if (file->data == MAP_FAILED) {
            file->data = NULL;
            close(file->fd);
            file->fd = -1;
            return READ_ERROR_MMAP;
        }
        file->is_mapped = 1;
    } else {
        file->data = (char*)malloc(file->size + 1);
        if (!file->data) {
            close(file->fd);
            file->fd = -1;
            return READ_ERROR_MEMORY;
        }

        ssize_t bytes_read = read(file->fd, file->data, file->size);
        if (bytes_read < 0 || (size_t)bytes_read != file->size) {
            free(file->data);
            file->data = NULL;
            close(file->fd);
            file->fd = -1;
            return READ_ERROR_OPEN;
        }
        file->data[file->size] = '\0';
        file->is_mapped = 0;
        close(file->fd);
        file->fd = -1;
    }

    return READ_SUCCESS;
}

FileList* filelist_create(void) {
    FileList* list = (FileList*)malloc(sizeof(FileList));
    if (!list) return NULL;

    list->files = (FileData*)malloc(sizeof(FileData) * INITIAL_FILE_CAPACITY);
    if (!list->files) {
        free(list);
        return NULL;
    }

    list->count = 0;
    list->capacity = INITIAL_FILE_CAPACITY;
    return list;
}

void filelist_free(FileList* list) {
    if (!list) return;

    for (size_t i = 0; i < list->count; i++) {
        file_close(&list->files[i]);
    }

    free(list->files);
    free(list);
}

int filelist_add(FileList* list, FileData* file) {
    if (!list || !file) return 0;

    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity * 2;
        FileData* new_files = (FileData*)realloc(list->files, sizeof(FileData) * new_capacity);
        if (!new_files) return 0;

        list->files = new_files;
        list->capacity = new_capacity;
    }

    list->files[list->count++] = *file;
    return 1;
}

int filelist_add_path(FileList* list, const char* filepath) {
    if (!list || !filepath) return 0;

    FileData* file = file_open(filepath);
    if (!file) return 0;

    ReadStatus status = file_read(file);
    if (status != READ_SUCCESS) {
        file_close(file);
        return 0;
    }

    if (!filelist_add(list, file)) {
        file_close(file);
        return 0;
    }

    return 1;
}

int traverse_directory(const char* dirpath, int recursive, FileCallback callback, void* userdata) {
    DIR* dir = opendir(dirpath);
    if (!dir) return 0;

    struct dirent* entry;
    char path[4096];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);

        struct stat st;
        if (stat(path, &st) != 0) {
            continue;
        }

        if (S_ISREG(st.st_mode)) {
            if (callback) {
                callback(path, userdata);
            }
        } else if (recursive && S_ISDIR(st.st_mode)) {
            traverse_directory(path, recursive, callback, userdata);
        }
    }

    closedir(dir);
    return 1;
}

size_t count_lines(const char* data, size_t size) {
    if (!data || size == 0) return 0;

    size_t count = 0;
    for (size_t i = 0; i < size; i++) {
        if (data[i] == '\n') {
            count++;
        }
    }

    if (size > 0 && data[size - 1] != '\n') {
        count++;
    }

    return count;
}

const char* find_line_start(const char* data, size_t size, size_t pos) {
    if (!data || pos >= size) return data;

    size_t i = pos;
    while (i > 0 && data[i - 1] != '\n') {
        i--;
    }

    return data + i;
}

const char* find_line_end(const char* data, size_t size, size_t pos) {
    if (!data || pos >= size) return data + size;

    size_t i = pos;
    while (i < size && data[i] != '\n') {
        i++;
    }

    return data + i;
}

size_t get_line_number(const char* data, size_t size, size_t pos) {
    if (!data || pos >= size) return 0;

    size_t line = 1;
    for (size_t i = 0; i < pos && i < size; i++) {
        if (data[i] == '\n') {
            line++;
        }
    }

    return line;
}