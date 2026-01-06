#include "include/file_reader.h"
#include "include/regex_simd.h"
#include "include/search.h"
#include "include/output.h"
#include "include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#define VERSION "1.0.0"

typedef struct {
    char** paths;
    size_t path_count;
    char* pattern;
    int recursive;
    int ignore_case;
    int use_regex;
    int color;
    int line_numbers;
    int show_filename;
    int quiet;
    int verbose;
    size_t num_threads;
    int color_set;
    int line_numbers_set;
} Config;

void config_init(Config* config) {
    config->paths = NULL;
    config->path_count = 0;
    config->pattern = NULL;
    config->recursive = 0;
    config->ignore_case = 0;
    config->use_regex = 0;
    config->color = 0;
    config->line_numbers = 0;
    config->show_filename = 0;
    config->quiet = 0;
    config->verbose = 0;
    config->num_threads = 1;
    config->color_set = 0;
    config->line_numbers_set = 0;
}

void config_free(Config* config) {
    if (!config) return;

    if (config->paths) {
        for (size_t i = 0; i < config->path_count; i++) {
            free(config->paths[i]);
        }
        free(config->paths);
    }

    if (config->pattern) {
        free(config->pattern);
    }
}

void print_usage(const char* program_name) {
    fprintf(stderr, "fastgrep %s - Ultra-fast grep replacement\n", VERSION);
    fprintf(stderr, "Usage: %s [OPTIONS] PATTERN [FILE...]\n", program_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Pattern Matching:\n");
    fprintf(stderr, "  -e, --regex            Use regex matching (default: ASCII substring)\n");
    fprintf(stderr, "  -i, --ignore-case      Case-insensitive search\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Search Options:\n");
    fprintf(stderr, "  -r, --recursive        Recursively search directories\n");
    fprintf(stderr, "      --threads <N>      Number of threads (default: 1)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Output Options:\n");
    fprintf(stderr, "  -n, --line-number      Show line numbers\n");
    fprintf(stderr, "      --no-line-number    Don't show line numbers\n");
    fprintf(stderr, "      --color            Highlight matches (default when TTY)\n");
    fprintf(stderr, "      --no-color          Don't highlight matches\n");
    fprintf(stderr, "  -q, --quiet            Quiet mode (only exit code matters)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Other Options:\n");
    fprintf(stderr, "  -v, --verbose          Verbose output\n");
    fprintf(stderr, "  -h, --help             Show this help message\n");
    fprintf(stderr, "      --version          Show version information\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s pattern file.txt\n", program_name);
    fprintf(stderr, "  %s -r pattern /path/to/dir\n", program_name);
    fprintf(stderr, "  %s -i -n pattern file.txt\n", program_name);
    fprintf(stderr, "  %s -e 'error.*[0-9]+' file.txt\n", program_name);
    fprintf(stderr, "  %s --threads 4 pattern *.log\n", program_name);
}

int parse_arguments(int argc, char** argv, Config* config) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    config->paths = (char**)malloc(sizeof(char*) * (argc - 1));
    if (!config->paths) {
        fprintf(stderr, "Memory allocation error\n");
        return 0;
    }

    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            config_free(config);
            exit(0);
        } else if (strcmp(argv[i], "--version") == 0) {
            printf("fastgrep %s\n", VERSION);
            config_free(config);
            exit(0);
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--recursive") == 0) {
            config->recursive = 1;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--ignore-case") == 0) {
            config->ignore_case = 1;
        } else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--regex") == 0) {
            config->use_regex = 1;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--line-number") == 0) {
            config->line_numbers = 1;
            config->line_numbers_set = 1;
        } else if (strcmp(argv[i], "--no-line-number") == 0) {
            config->line_numbers = 0;
            config->line_numbers_set = 1;
        } else if (strcmp(argv[i], "--color") == 0) {
            config->color = 1;
            config->color_set = 1;
        } else if (strcmp(argv[i], "--no-color") == 0) {
            config->color = 0;
            config->color_set = 1;
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
            config->quiet = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            config->verbose = 1;
        } else if (strcmp(argv[i], "--threads") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --threads requires an argument\n");
                return 0;
            }
            config->num_threads = (size_t)atoi(argv[i + 1]);
            i++;
        } else if (argv[i][0] == '-' && strlen(argv[i]) > 1) {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            return 0;
        } else {
            if (!config->pattern) {
                config->pattern = strdup(argv[i]);
            } else {
                config->paths[config->path_count++] = strdup(argv[i]);
            }
        }
        i++;
    }

    if (!config->pattern) {
        fprintf(stderr, "Error: No pattern specified\n");
        return 0;
    }

    if (config->path_count == 0) {
        config->paths[config->path_count++] = strdup("-");
    }

    if (!config->color_set) {
        config->color = isatty(fileno(stdout)) ? 1 : 0;
    }

    if (!config->line_numbers_set) {
        config->line_numbers = 0;
    }

    if (config->path_count > 1 || config->recursive) {
        config->show_filename = 1;
    }

    return 1;
}

int is_directory(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}

typedef struct {
    FileList* filelist;
    int error;
} DirectoryTraversalData;

void add_file_to_list(const char* filepath, void* userdata) {
    DirectoryTraversalData* data = (DirectoryTraversalData*)userdata;

    FileData* file = file_open(filepath);
    if (!file) {
        data->error = 1;
        return;
    }

    ReadStatus status = file_read(file);
    if (status != READ_SUCCESS) {
        file_close(file);
        return;
    }

    filelist_add(data->filelist, file);
}

int main(int argc, char** argv) {
    Config config;
    config_init(&config);

    if (!parse_arguments(argc, argv, &config)) {
        config_free(&config);
        return 2;
    }

    Logger* logger = logger_create(config.verbose ? LOG_DEBUG : LOG_WARN);
    logger_enable(logger, config.verbose);

    if (config.verbose) {
        logger_info(logger, "fastgrep %s starting", VERSION);
        logger_info(logger, "Pattern: %s", config.pattern);
        logger_info(logger, "Recursive: %s", config.recursive ? "yes" : "no");
        logger_info(logger, "Case insensitive: %s", config.ignore_case ? "yes" : "no");
        logger_info(logger, "Regex mode: %s", config.use_regex ? "yes" : "no");
        logger_info(logger, "Threads: %zu", config.num_threads);
    }

    FileList* filelist = filelist_create();
    if (!filelist) {
        output_error("Memory allocation error");
        config_free(&config);
        logger_free(logger);
        return 2;
    }

    DirectoryTraversalData traversal_data;
    traversal_data.filelist = filelist;
    traversal_data.error = 0;

    for (size_t i = 0; i < config.path_count; i++) {
        const char* path = config.paths[i];

        if (strcmp(path, "-") == 0) {
            char buffer[8192];
            size_t total_size = 0;
            size_t buffer_capacity = 8192;
            char* content = (char*)malloc(buffer_capacity);

            while (fgets(buffer, sizeof(buffer), stdin)) {
                size_t len = strlen(buffer);
                if (total_size + len >= buffer_capacity) {
                    buffer_capacity *= 2;
                    char* new_content = (char*)realloc(content, buffer_capacity);
                    if (!new_content) {
                        free(content);
                        output_error("Memory allocation error");
                        config_free(&config);
                        logger_free(logger);
                        return 2;
                    }
                    content = new_content;
                }
                memcpy(content + total_size, buffer, len);
                total_size += len;
            }

            FileData* file = file_open("(stdin)");
            if (!file) {
                free(content);
                output_error("Memory allocation error");
                config_free(&config);
                logger_free(logger);
                return 2;
            }

            file->data = content;
            file->size = total_size;
            file->is_mapped = 0;
            file->fd = -1;

            filelist_add(filelist, file);
        } else if (is_directory(path)) {
            if (!config.recursive) {
                output_error("Path is a directory, use -r to search recursively");
                config_free(&config);
                logger_free(logger);
                filelist_free(filelist);
                return 2;
            }
            traverse_directory(path, config.recursive, add_file_to_list, &traversal_data);
        } else {
            FileData* file = file_open(path);
            if (!file) {
                fprintf(stderr, "fgrep: %s: %s\n", path, strerror(errno));
                continue;
            }

            ReadStatus status = file_read(file);
            if (status != READ_SUCCESS) {
                if (status == READ_ERROR_DIRECTORY) {
                    fprintf(stderr, "fgrep: %s: is a directory\n", path);
                } else {
                    fprintf(stderr, "fgrep: %s: %s\n", path, strerror(errno));
                }
                file_close(file);
                continue;
            }

            filelist_add(filelist, file);
        }
    }

    if (config.verbose) {
        logger_info(logger, "Loaded %zu files", filelist->count);
    }

    if (filelist->count == 0) {
        output_error("No files to search");
        config_free(&config);
        logger_free(logger);
        filelist_free(filelist);
        return 1;
    }

    Pattern* pattern = pattern_create(config.pattern, config.ignore_case, config.use_regex);
    if (!pattern) {
        output_error("Invalid pattern");
        config_free(&config);
        logger_free(logger);
        filelist_free(filelist);
        return 2;
    }

    OutputConfig output_config;
    output_init(&output_config);
    output_set_color(&output_config, config.color);
    output_set_line_numbers(&output_config, config.line_numbers);
    output_set_show_filename(&output_config, config.show_filename);
    output_set_quiet(&output_config, config.quiet);

    MatchList** results = NULL;
    logger_timer_start(logger);

    int success = search_multiple_files(pattern, filelist, config.num_threads, &results);

    logger_timer_stop(logger);

    if (config.verbose) {
        logger_timer_print(logger);
    }

    int exit_code = 1;
    if (success && results) {
        size_t total_matches = 0;
        for (size_t i = 0; i < filelist->count; i++) {
            if (results[i] && results[i]->count > 0) {
                if (!config.quiet) {
                    output_matches(&output_config,
                                   filelist->files[i]->filepath,
                                   filelist->files[i]->data,
                                   filelist->files[i]->size,
                                   results[i]);
                }
                total_matches += results[i]->count;
            }
            if (results[i]) {
                matchlist_free(results[i]);
            }
        }

        if (total_matches > 0) {
            exit_code = 0;
        }

        if (config.verbose) {
            logger_info(logger, "Found %zu matches", total_matches);
        }

        free(results);
    }

    pattern_free(pattern);
    config_free(&config);
    logger_free(logger);
    filelist_free(filelist);

    return exit_code;
}