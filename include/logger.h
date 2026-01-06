#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <sys/time.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

typedef struct {
    FILE* output;
    LogLevel level;
    int enabled;
    struct timeval start_time;
} Logger;

Logger* logger_create(LogLevel level);
void logger_free(Logger* logger);

void logger_set_level(Logger* logger, LogLevel level);
void logger_set_output(Logger* logger, FILE* output);
void logger_enable(Logger* logger, int enabled);

void logger_log(Logger* logger, LogLevel level, const char* format, ...);
void logger_debug(Logger* logger, const char* format, ...);
void logger_info(Logger* logger, const char* format, ...);
void logger_warn(Logger* logger, const char* format, ...);
void logger_error(Logger* logger, const char* format, ...);

void logger_timer_start(Logger* logger);
void logger_timer_stop(Logger* logger);
void logger_timer_print(Logger* logger);

#endif