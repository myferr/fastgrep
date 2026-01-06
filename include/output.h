#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdio.h>
#include <stddef.h>
#include "../include/file_reader.h"
#include "../include/regex_simd.h"

typedef struct {
    int color;
    int line_numbers;
    int show_filename;
    int quiet;
    FILE* output;
} OutputConfig;

typedef enum {
    COLOR_RESET,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE,
    COLOR_BOLD
} ColorCode;

void output_init(OutputConfig* config);
void output_free(OutputConfig* config);

void output_set_color(OutputConfig* config, int enable);
void output_set_line_numbers(OutputConfig* config, int enable);
void output_set_show_filename(OutputConfig* config, int enable);
void output_set_quiet(OutputConfig* config, int enable);

void output_color_start(OutputConfig* config, ColorCode color);
void output_color_end(OutputConfig* config);

void output_match(OutputConfig* config, const char* filepath, const char* data, size_t size, const Match* match);
void output_matches(OutputConfig* config, const char* filepath, const char* data, size_t size, const MatchList* matches);

void output_error(const char* message);
void output_info(const char* message);

int is_stdout_tty(void);

#endif