#include "../include/output.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char* COLOR_CODES[] = {
    "\033[0m",
    "\033[31m",
    "\033[32m",
    "\033[33m",
    "\033[34m",
    "\033[35m",
    "\033[36m",
    "\033[37m",
    "\033[1m"
};

void output_init(OutputConfig* config) {
    if (!config) return;

    config->color = is_stdout_tty() ? 1 : 0;
    config->line_numbers = 0;
    config->show_filename = 0;
    config->quiet = 0;
    config->output = stdout;
}

void output_free(OutputConfig* config) {
    (void)config;
}

void output_set_color(OutputConfig* config, int enable) {
    if (!config) return;
    config->color = enable ? 1 : 0;
}

void output_set_line_numbers(OutputConfig* config, int enable) {
    if (!config) return;
    config->line_numbers = enable ? 1 : 0;
}

void output_set_show_filename(OutputConfig* config, int enable) {
    if (!config) return;
    config->show_filename = enable ? 1 : 0;
}

void output_set_quiet(OutputConfig* config, int enable) {
    if (!config) return;
    config->quiet = enable ? 1 : 0;
}

void output_color_start(OutputConfig* config, ColorCode color) {
    if (!config || !config->color || !config->output) return;

    if (color >= 0 && color < sizeof(COLOR_CODES) / sizeof(COLOR_CODES[0])) {
        fputs(COLOR_CODES[color], config->output);
    }
}

void output_color_end(OutputConfig* config) {
    if (!config || !config->color || !config->output) return;

    fputs(COLOR_CODES[COLOR_RESET], config->output);
}

void output_match(OutputConfig* config, const char* filepath, const char* data, size_t size, const Match* match) {
    if (!config || !data || !match || config->quiet) return;
    if (match->start >= size || match->end > size) return;

    FILE* out = config->output;

    const char* line_start = find_line_start(data, size, match->start);
    const char* line_end = find_line_end(data, size, match->start);
    size_t line_len = line_end - line_start;

    if (config->show_filename && filepath) {
        fprintf(out, "%s:", filepath);
    }

    if (config->line_numbers) {
        fprintf(out, "%zu:", match->line_num);
    }

    if (config->color) {
        size_t match_offset = match->start - (line_start - data);
        size_t match_len = match->end - match->start;

        fwrite(line_start, 1, match_offset, out);

        output_color_start(config, COLOR_RED);
        fwrite(line_start + match_offset, 1, match_len, out);
        output_color_end(config);

        fwrite(line_start + match_offset + match_len, 1, line_len - match_offset - match_len, out);
    } else {
        fwrite(line_start, 1, line_len, out);
    }

    fputc('\n', out);
}

void output_matches(OutputConfig* config, const char* filepath, const char* data, size_t size, const MatchList* matches) {
    if (!config || !data || !matches) return;

    for (size_t i = 0; i < matches->count; i++) {
        output_match(config, filepath, data, size, &matches->matches[i]);
    }
}

void output_error(const char* message) {
    if (!message) return;

    fprintf(stderr, "fgrep: %s\n", message);
}

void output_info(const char* message) {
    if (!message) return;

    fprintf(stderr, "fgrep: %s\n", message);
}

int is_stdout_tty(void) {
    return isatty(fileno(stdout));
}