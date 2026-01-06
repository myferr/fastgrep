#include "../include/logger.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static const char* LOG_LEVEL_NAMES[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR"
};

static const char* LOG_LEVEL_COLORS[] = {
    "\033[36m",
    "\033[32m",
    "\033[33m",
    "\033[31m"
};

static const char* COLOR_RESET = "\033[0m";

Logger* logger_create(LogLevel level) {
    Logger* logger = (Logger*)malloc(sizeof(Logger));
    if (!logger) return NULL;

    logger->output = stderr;
    logger->level = level;
    logger->enabled = 1;
    gettimeofday(&logger->start_time, NULL);

    return logger;
}

void logger_free(Logger* logger) {
    free(logger);
}

void logger_set_level(Logger* logger, LogLevel level) {
    if (!logger) return;
    logger->level = level;
}

void logger_set_output(Logger* logger, FILE* output) {
    if (!logger) return;
    logger->output = output;
}

void logger_enable(Logger* logger, int enabled) {
    if (!logger) return;
    logger->enabled = enabled ? 1 : 0;
}

void logger_log(Logger* logger, LogLevel level, const char* format, ...) {
    if (!logger || !logger->enabled || level < logger->level) return;
    if (!format) return;

    va_list args;
    va_start(args, format);

    int is_tty = isatty(fileno(logger->output));

    if (is_tty) {
        fprintf(logger->output, "%s[%s]%s ", LOG_LEVEL_COLORS[level], LOG_LEVEL_NAMES[level], COLOR_RESET);
    } else {
        fprintf(logger->output, "[%s] ", LOG_LEVEL_NAMES[level]);
    }

    vfprintf(logger->output, format, args);
    fprintf(logger->output, "\n");

    va_end(args);
}

void logger_debug(Logger* logger, const char* format, ...) {
    if (!logger || !logger->enabled || LOG_DEBUG < logger->level) return;
    if (!format) return;

    va_list args;
    va_start(args, format);
    logger_log(logger, LOG_DEBUG, format, args);
    va_end(args);
}

void logger_info(Logger* logger, const char* format, ...) {
    if (!logger || !logger->enabled || LOG_INFO < logger->level) return;
    if (!format) return;

    va_list args;
    va_start(args, format);
    logger_log(logger, LOG_INFO, format, args);
    va_end(args);
}

void logger_warn(Logger* logger, const char* format, ...) {
    if (!logger || !logger->enabled || LOG_WARN < logger->level) return;
    if (!format) return;

    va_list args;
    va_start(args, format);
    logger_log(logger, LOG_WARN, format, args);
    va_end(args);
}

void logger_error(Logger* logger, const char* format, ...) {
    if (!logger || !logger->enabled || LOG_ERROR < logger->level) return;
    if (!format) return;

    va_list args;
    va_start(args, format);
    logger_log(logger, LOG_ERROR, format, args);
    va_end(args);
}

void logger_timer_start(Logger* logger) {
    if (!logger) return;
    gettimeofday(&logger->start_time, NULL);
}

void logger_timer_stop(Logger* logger) {
    (void)logger;
}

void logger_timer_print(Logger* logger) {
    if (!logger || !logger->enabled) return;

    struct timeval end_time;
    gettimeofday(&end_time, NULL);

    double elapsed = (end_time.tv_sec - logger->start_time.tv_sec) * 1000.0;
    elapsed += (end_time.tv_usec - logger->start_time.tv_usec) / 1000.0;

    logger_info(logger, "Elapsed time: %.2f ms", elapsed);
}