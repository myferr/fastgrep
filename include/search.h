#ifndef SEARCH_H
#define SEARCH_H

#include <pthread.h>
#include <stddef.h>
#include "../include/file_reader.h"
#include "../include/regex_simd.h"

typedef struct {
    const Pattern* pattern;
    const FileData* file;
    MatchList* matches;
    int file_index;
} SearchTask;

typedef struct {
    SearchTask* tasks;
    size_t count;
    size_t capacity;
    pthread_mutex_t mutex;
    size_t next_task;
} TaskQueue;

typedef struct {
    TaskQueue* queue;
    pthread_t* threads;
    size_t num_threads;
    size_t files_searched;
    size_t total_matches;
    pthread_mutex_t result_mutex;
} SearchContext;

TaskQueue* taskqueue_create(void);
void taskqueue_free(TaskQueue* queue);
int taskqueue_add(TaskQueue* queue, const Pattern* pattern, const FileData* file, int file_index);
SearchTask* taskqueue_get_next(TaskQueue* queue);

SearchContext* search_context_create(size_t num_threads);
void search_context_free(SearchContext* context);
int search_context_run(SearchContext* context, TaskQueue* queue);
void* search_worker(void* arg);

int search_single_file(const Pattern* pattern, const FileData* file, MatchList* matches);
int search_multiple_files(const Pattern* pattern, const FileList* files, size_t num_threads, MatchList*** results);

#endif