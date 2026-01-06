#ifndef REGEX_SIMD_H
#define REGEX_SIMD_H

#include <stddef.h>
#include <regex.h>

typedef enum {
    MATCH_ASCII,
    MATCH_REGEX
} MatchType;

typedef struct {
    char* pattern;
    size_t pattern_len;
    int case_insensitive;
    MatchType type;
    regex_t regex_compiled;
    int is_regex_compiled;
} Pattern;

typedef struct {
    size_t start;
    size_t end;
    size_t line_num;
} Match;

typedef struct {
    Match* matches;
    size_t count;
    size_t capacity;
} MatchList;

Pattern* pattern_create(const char* pattern_str, int case_insensitive, int use_regex);
void pattern_free(Pattern* pattern);

MatchList* matchlist_create(void);
void matchlist_free(MatchList* list);
int matchlist_add(MatchList* list, size_t start, size_t end, size_t line_num);

int pattern_match_ascii(const Pattern* pattern, const char* data, size_t size, size_t pos);
int pattern_match_ascii_case(const Pattern* pattern, const char* data, size_t size, size_t pos);
int pattern_match_regex(const Pattern* pattern, const char* data, size_t size, size_t pos);

int search_pattern(const Pattern* pattern, const char* data, size_t size, MatchList* matches);
int search_pattern_ascii(const Pattern* pattern, const char* data, size_t size, MatchList* matches);
int search_pattern_regex(const Pattern* pattern, const char* data, size_t size, MatchList* matches);

#ifdef __SSE4_2__
int search_pattern_sse42(const Pattern* pattern, const char* data, size_t size, MatchList* matches);
#endif

#ifdef __AVX2__
int search_pattern_avx2(const Pattern* pattern, const char* data, size_t size, MatchList* matches);
#endif

int is_simd_available(void);

#endif