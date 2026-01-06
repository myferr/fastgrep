#include "../include/regex_simd.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#define INITIAL_MATCH_CAPACITY 1024

Pattern* pattern_create(const char* pattern_str, int case_insensitive, int use_regex) {
    if (!pattern_str) return NULL;

    Pattern* pattern = (Pattern*)malloc(sizeof(Pattern));
    if (!pattern) return NULL;

    pattern->pattern = strdup(pattern_str);
    if (!pattern->pattern) {
        free(pattern);
        return NULL;
    }

    pattern->pattern_len = strlen(pattern_str);
    pattern->case_insensitive = case_insensitive;
    pattern->type = use_regex ? MATCH_REGEX : MATCH_ASCII;
    pattern->is_regex_compiled = 0;

    if (pattern->type == MATCH_REGEX) {
        int flags = REG_EXTENDED;
        if (case_insensitive) {
            flags |= REG_ICASE;
        }

        if (regcomp(&pattern->regex_compiled, pattern->pattern, flags) != 0) {
            free(pattern->pattern);
            free(pattern);
            return NULL;
        }

        pattern->is_regex_compiled = 1;
    }

    return pattern;
}

void pattern_free(Pattern* pattern) {
    if (!pattern) return;

    if (pattern->is_regex_compiled) {
        regfree(&pattern->regex_compiled);
    }

    if (pattern->pattern) {
        free(pattern->pattern);
    }

    free(pattern);
}

MatchList* matchlist_create(void) {
    MatchList* list = (MatchList*)malloc(sizeof(MatchList));
    if (!list) return NULL;

    list->matches = (Match*)malloc(sizeof(Match) * INITIAL_MATCH_CAPACITY);
    if (!list->matches) {
        free(list);
        return NULL;
    }

    list->count = 0;
    list->capacity = INITIAL_MATCH_CAPACITY;
    return list;
}

void matchlist_free(MatchList* list) {
    if (!list) return;

    if (list->matches) {
        free(list->matches);
    }

    free(list);
}

int matchlist_add(MatchList* list, size_t start, size_t end, size_t line_num) {
    if (!list) return 0;

    if (list->count >= list->capacity) {
        size_t new_capacity = list->capacity * 2;
        Match* new_matches = (Match*)realloc(list->matches, sizeof(Match) * new_capacity);
        if (!new_matches) return 0;

        list->matches = new_matches;
        list->capacity = new_capacity;
    }

    list->matches[list->count].start = start;
    list->matches[list->count].end = end;
    list->matches[list->count].line_num = line_num;
    list->count++;

    return 1;
}

int pattern_match_ascii(const Pattern* pattern, const char* data, size_t size, size_t pos) {
    if (!pattern || !data || pos >= size || pos + pattern->pattern_len > size) {
        return 0;
    }

    return memcmp(data + pos, pattern->pattern, pattern->pattern_len) == 0;
}

int pattern_match_ascii_case(const Pattern* pattern, const char* data, size_t size, size_t pos) {
    if (!pattern || !data || pos >= size || pos + pattern->pattern_len > size) {
        return 0;
    }

    for (size_t i = 0; i < pattern->pattern_len; i++) {
        if (tolower((unsigned char)pattern->pattern[i]) != tolower((unsigned char)data[pos + i])) {
            return 0;
        }
    }

    return 1;
}

int pattern_match_regex(const Pattern* pattern, const char* data, size_t size, size_t pos) {
    if (!pattern || !data || !pattern->is_regex_compiled || pos > size) {
        return 0;
    }

    regmatch_t match;
    int result = regexec(&pattern->regex_compiled, data + pos, 1, &match, 0);

    return result == 0;
}

int search_pattern_ascii(const Pattern* pattern, const char* data, size_t size, MatchList* matches) {
    if (!pattern || !data || !matches) return 0;

    size_t pos = 0;
    size_t line_num = 1;
    int last_was_newline = 0;

    while (pos < size) {
        int found = 0;

        if (pattern->case_insensitive) {
            found = pattern_match_ascii_case(pattern, data, size, pos);
        } else {
            found = pattern_match_ascii(pattern, data, size, pos);
        }

        if (found) {
            matchlist_add(matches, pos, pos + pattern->pattern_len, line_num);
            pos++;
        } else {
            if (data[pos] == '\n') {
                line_num++;
                last_was_newline = 1;
            } else {
                last_was_newline = 0;
            }
            pos++;
        }
    }

    return matches->count > 0;
}

int search_pattern_regex(const Pattern* pattern, const char* data, size_t size, MatchList* matches) {
    if (!pattern || !data || !matches || !pattern->is_regex_compiled) return 0;

    size_t pos = 0;
    size_t line_num = 1;
    regmatch_t regmatch;

    while (pos <= size) {
        int result = regexec(&pattern->regex_compiled, data + pos, 1, &regmatch, 0);

        if (result != 0 || regmatch.rm_so == -1) {
            break;
        }

        size_t match_start = pos + regmatch.rm_so;
        size_t match_end = pos + regmatch.rm_eo;

        matchlist_add(matches, match_start, match_end, line_num);

        pos = match_end ? match_end : pos + 1;
    }

    return matches->count > 0;
}

int search_pattern(const Pattern* pattern, const char* data, size_t size, MatchList* matches) {
    if (!pattern || !data || !matches) return 0;

#ifdef __AVX2__
    if (pattern->type == MATCH_ASCII && !pattern->case_insensitive && is_simd_available()) {
        if (search_pattern_avx2(pattern, data, size, matches)) {
            return 1;
        }
    }
#endif

#ifdef __SSE4_2__
    if (pattern->type == MATCH_ASCII && !pattern->case_insensitive && is_simd_available()) {
        if (search_pattern_sse42(pattern, data, size, matches)) {
            return 1;
        }
    }
#endif

    if (pattern->type == MATCH_ASCII) {
        return search_pattern_ascii(pattern, data, size, matches);
    } else {
        return search_pattern_regex(pattern, data, size, matches);
    }
}

#ifdef __SSE4_2__
#include <nmmintrin.h>

int search_pattern_sse42(const Pattern* pattern, const char* data, size_t size, MatchList* matches) {
    if (!pattern || !data || !matches || pattern->pattern_len == 0) return 0;
    if (pattern->pattern_len > 16) {
        return search_pattern_ascii(pattern, data, size, matches);
    }

    __m128i pattern_vec = _mm_loadu_si128((const __m128i*)pattern->pattern);
    size_t pos = 0;
    size_t line_num = 1;

    while (pos + 16 <= size) {
        __m128i data_vec = _mm_loadu_si128((const __m128i*)(data + pos));
        __m128i cmp = _mm_cmpeq_epi8(data_vec, pattern_vec);

        unsigned mask = _mm_movemask_epi8(cmp);

        if (mask != 0) {
            for (int i = 0; i < 16; i++) {
                if (mask & (1 << i)) {
                    if (pos + i + pattern->pattern_len <= size) {
                        if (memcmp(data + pos + i, pattern->pattern, pattern->pattern_len) == 0) {
                            matchlist_add(matches, pos + i, pos + i + pattern->pattern_len, line_num);
                        }
                    }
                }
            }
        }

        for (int i = 0; i < 16 && pos + i < size; i++) {
            if (data[pos + i] == '\n') {
                line_num++;
            }
        }

        pos += 16;
    }

    while (pos < size) {
        if (pattern_match_ascii(pattern, data, size, pos)) {
            matchlist_add(matches, pos, pos + pattern->pattern_len, line_num);
        }
        if (data[pos] == '\n') {
            line_num++;
        }
        pos++;
    }

    return matches->count > 0;
}
#endif

#ifdef __AVX2__
#include <immintrin.h>

int search_pattern_avx2(const Pattern* pattern, const char* data, size_t size, MatchList* matches) {
    if (!pattern || !data || !matches || pattern->pattern_len == 0) return 0;
    if (pattern->pattern_len > 32) {
        return search_pattern_ascii(pattern, data, size, matches);
    }

    __m256i pattern_vec = _mm256_loadu_si256((const __m256i*)pattern->pattern);
    size_t pos = 0;
    size_t line_num = 1;

    while (pos + 32 <= size) {
        __m256i data_vec = _mm256_loadu_si256((const __m256i*)(data + pos));
        __m256i cmp = _mm256_cmpeq_epi8(data_vec, pattern_vec);

        unsigned mask = _mm256_movemask_epi8(cmp);

        if (mask != 0) {
            for (int i = 0; i < 32; i++) {
                if (mask & (1 << i)) {
                    if (pos + i + pattern->pattern_len <= size) {
                        if (memcmp(data + pos + i, pattern->pattern, pattern->pattern_len) == 0) {
                            matchlist_add(matches, pos + i, pos + i + pattern->pattern_len, line_num);
                        }
                    }
                }
            }
        }

        for (int i = 0; i < 32 && pos + i < size; i++) {
            if (data[pos + i] == '\n') {
                line_num++;
            }
        }

        pos += 32;
    }

    while (pos < size) {
        if (pattern_match_ascii(pattern, data, size, pos)) {
            matchlist_add(matches, pos, pos + pattern->pattern_len, line_num);
        }
        if (data[pos] == '\n') {
            line_num++;
        }
        pos++;
    }

    return matches->count > 0;
}
#endif

int is_simd_available(void) {
#ifdef __SSE4_2__
    return 1;
#elif defined(__AVX2__)
    return 1;
#else
    return 0;
#endif
}