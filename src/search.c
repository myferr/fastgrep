#include "../include/search.h"
#include <stdlib.h>
#include <string.h>

TaskQueue* taskqueue_create(void) {
    TaskQueue* queue = (TaskQueue*)malloc(sizeof(TaskQueue));
    if (!queue) return NULL;

    queue->tasks = NULL;
    queue->count = 0;
    queue->capacity = 0;
    queue->next_task = 0;

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue);
        return NULL;
    }

    return queue;
}

void taskqueue_free(TaskQueue* queue) {
    if (!queue) return;

    if (queue->tasks) {
        free(queue->tasks);
    }

    pthread_mutex_destroy(&queue->mutex);
    free(queue);
}

int taskqueue_add(TaskQueue* queue, const Pattern* pattern, const FileData* file, int file_index) {
    if (!queue || !pattern || !file) return 0;

    if (queue->count >= queue->capacity) {
        size_t new_capacity = queue->capacity == 0 ? 16 : queue->capacity * 2;
        SearchTask* new_tasks = (SearchTask*)realloc(queue->tasks, sizeof(SearchTask) * new_capacity);
        if (!new_tasks) return 0;

        queue->tasks = new_tasks;
        queue->capacity = new_capacity;
    }

    queue->tasks[queue->count].pattern = pattern;
    queue->tasks[queue->count].file = file;
    queue->tasks[queue->count].matches = matchlist_create();
    if (!queue->tasks[queue->count].matches) {
        return 0;
    }
    queue->tasks[queue->count].file_index = file_index;
    queue->count++;

    return 1;
}

SearchTask* taskqueue_get_next(TaskQueue* queue) {
    if (!queue) return NULL;

    pthread_mutex_lock(&queue->mutex);

    if (queue->next_task >= queue->count) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    SearchTask* task = &queue->tasks[queue->next_task++];
    pthread_mutex_unlock(&queue->mutex);

    return task;
}

SearchContext* search_context_create(size_t num_threads) {
    SearchContext* context = (SearchContext*)malloc(sizeof(SearchContext));
    if (!context) return NULL;

    context->queue = NULL;
    context->threads = NULL;
    context->num_threads = num_threads > 0 ? num_threads : 1;
    context->files_searched = 0;
    context->total_matches = 0;

    if (pthread_mutex_init(&context->result_mutex, NULL) != 0) {
        free(context);
        return NULL;
    }

    context->threads = (pthread_t*)malloc(sizeof(pthread_t) * context->num_threads);
    if (!context->threads) {
        pthread_mutex_destroy(&context->result_mutex);
        free(context);
        return NULL;
    }

    return context;
}

void search_context_free(SearchContext* context) {
    if (!context) return;

    if (context->threads) {
        free(context->threads);
    }

    pthread_mutex_destroy(&context->result_mutex);
    free(context);
}

void* search_worker(void* arg) {
    SearchContext* context = (SearchContext*)arg;
    if (!context || !context->queue) return NULL;

    SearchTask* task;
    while ((task = taskqueue_get_next(context->queue)) != NULL) {
        int found = search_pattern(task->pattern, task->file->data, task->file->size, task->matches);

        pthread_mutex_lock(&context->result_mutex);
        context->files_searched++;
        context->total_matches += task->matches->count;
        pthread_mutex_unlock(&context->result_mutex);
    }

    return NULL;
}

int search_context_run(SearchContext* context, TaskQueue* queue) {
    if (!context || !queue) return 0;

    context->queue = queue;

    for (size_t i = 0; i < context->num_threads; i++) {
        if (pthread_create(&context->threads[i], NULL, search_worker, context) != 0) {
            return 0;
        }
    }

    for (size_t i = 0; i < context->num_threads; i++) {
        pthread_join(context->threads[i], NULL);
    }

    return 1;
}

int search_single_file(const Pattern* pattern, const FileData* file, MatchList* matches) {
    if (!pattern || !file || !matches) return 0;

    return search_pattern(pattern, file->data, file->size, matches);
}

int search_multiple_files(const Pattern* pattern, const FileList* files, size_t num_threads, MatchList** results) {
    if (!pattern || !files || !results) return 0;

    TaskQueue* queue = taskqueue_create();
    if (!queue) return 0;

    for (size_t i = 0; i < files->count; i++) {
        if (!taskqueue_add(queue, pattern, &files->files[i], (int)i)) {
            taskqueue_free(queue);
            return 0;
        }
    }

    SearchContext* context = search_context_create(num_threads);
    if (!context) {
        taskqueue_free(queue);
        return 0;
    }

    int success = search_context_run(context, queue);

    *results = (MatchList*)malloc(sizeof(MatchList) * queue->count);
    if (!*results) {
        search_context_free(context);
        taskqueue_free(queue);
        return 0;
    }

    for (size_t i = 0; i < queue->count; i++) {
        (*results)[i] = *queue->tasks[i].matches;
        queue->tasks[i].matches = NULL;
    }

    search_context_free(context);
    taskqueue_free(queue);

    return success;
}