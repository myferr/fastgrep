#include "../include/regex_simd.h"
#include "../include/file_reader.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int test_pattern_create(void) {
    Pattern* pattern = pattern_create("hello", 0, 0);
    if (!pattern) {
        printf("FAILED: pattern_create returned NULL\n");
        return 0;
    }

    if (strcmp(pattern->pattern, "hello") != 0) {
        printf("FAILED: pattern pattern mismatch\n");
        pattern_free(pattern);
        return 0;
    }

    if (pattern->pattern_len != 5) {
        printf("FAILED: pattern length mismatch\n");
        pattern_free(pattern);
        return 0;
    }

    pattern_free(pattern);
    printf("PASSED: test_pattern_create\n");
    return 1;
}

int test_pattern_case_insensitive(void) {
    Pattern* pattern = pattern_create("HELLO", 1, 0);
    if (!pattern) {
        printf("FAILED: pattern_create returned NULL\n");
        return 0;
    }

    if (!pattern->case_insensitive) {
        printf("FAILED: pattern case_insensitive not set\n");
        pattern_free(pattern);
        return 0;
    }

    const char* data = "hello HeLLo HELLO";
    MatchList* matches = matchlist_create();
    if (!matches) {
        pattern_free(pattern);
        printf("FAILED: matchlist_create returned NULL\n");
        return 0;
    }

    int found = search_pattern_ascii(pattern, data, strlen(data), matches);

    if (!found) {
        printf("FAILED: case insensitive search failed\n");
        pattern_free(pattern);
        matchlist_free(matches);
        return 0;
    }

    if (matches->count != 3) {
        printf("FAILED: expected 3 matches, got %zu\n", matches->count);
        pattern_free(pattern);
        matchlist_free(matches);
        return 0;
    }

    pattern_free(pattern);
    matchlist_free(matches);
    printf("PASSED: test_pattern_case_insensitive\n");
    return 1;
}

int test_pattern_regex(void) {
    Pattern* pattern = pattern_create("error[0-9]+", 0, 1);
    if (!pattern) {
        printf("FAILED: regex pattern_create returned NULL\n");
        return 0;
    }

    if (!pattern->is_regex_compiled) {
        printf("FAILED: regex not compiled\n");
        pattern_free(pattern);
        return 0;
    }

    const char* data = "error123 error456 error789";
    MatchList* matches = matchlist_create();
    if (!matches) {
        pattern_free(pattern);
        printf("FAILED: matchlist_create returned NULL\n");
        return 0;
    }

    int found = search_pattern_regex(pattern, data, strlen(data), matches);

    if (!found) {
        printf("FAILED: regex search failed\n");
        pattern_free(pattern);
        matchlist_free(matches);
        return 0;
    }

    if (matches->count != 3) {
        printf("FAILED: expected 3 regex matches, got %zu\n", matches->count);
        pattern_free(pattern);
        matchlist_free(matches);
        return 0;
    }

    pattern_free(pattern);
    matchlist_free(matches);
    printf("PASSED: test_pattern_regex\n");
    return 1;
}

int test_line_counting(void) {
    const char* data = "line1\nline2\nline3";
    size_t count = count_lines(data, strlen(data));

    if (count != 3) {
        printf("FAILED: expected 3 lines, got %zu\n", count);
        return 0;
    }

    count = count_lines("line1", 5);
    if (count != 1) {
        printf("FAILED: expected 1 line, got %zu\n", count);
        return 0;
    }

    printf("PASSED: test_line_counting\n");
    return 1;
}

int test_line_number_at_position(void) {
    const char* data = "line1\nline2\nline3\nline4";
    size_t line_num = get_line_number(data, strlen(data), 0);
    if (line_num != 1) {
        printf("FAILED: expected line 1, got %zu\n", line_num);
        return 0;
    }

    line_num = get_line_number(data, strlen(data), 6);
    if (line_num != 2) {
        printf("FAILED: expected line 2, got %zu\n", line_num);
        return 0;
    }

    line_num = get_line_number(data, strlen(data), 12);
    if (line_num != 3) {
        printf("FAILED: expected line 3, got %zu\n", line_num);
        return 0;
    }

    printf("PASSED: test_line_number_at_position\n");
    return 1;
}

int test_matchlist_operations(void) {
    MatchList* list = matchlist_create();
    if (!list) {
        printf("FAILED: matchlist_create returned NULL\n");
        return 0;
    }

    int result = matchlist_add(list, 10, 15, 5);
    if (!result) {
        printf("FAILED: matchlist_add failed\n");
        matchlist_free(list);
        return 0;
    }

    if (list->count != 1) {
        printf("FAILED: expected 1 match, got %zu\n", list->count);
        matchlist_free(list);
        return 0;
    }

    if (list->matches[0].start != 10 || list->matches[0].end != 15) {
        printf("FAILED: match start/end mismatch\n");
        matchlist_free(list);
        return 0;
    }

    for (int i = 0; i < 2000; i++) {
        matchlist_add(list, i, i + 5, i);
    }

    if (list->count != 2001) {
        printf("FAILED: expected 2001 matches, got %zu\n", list->count);
        matchlist_free(list);
        return 0;
    }

    matchlist_free(list);
    printf("PASSED: test_matchlist_operations\n");
    return 1;
}

int test_pattern_not_found(void) {
    Pattern* pattern = pattern_create("xyz", 0, 0);
    if (!pattern) {
        printf("FAILED: pattern_create returned NULL\n");
        return 0;
    }

    const char* data = "hello world";
    MatchList* matches = matchlist_create();
    if (!matches) {
        pattern_free(pattern);
        printf("FAILED: matchlist_create returned NULL\n");
        return 0;
    }

    int found = search_pattern_ascii(pattern, data, strlen(data), matches);

    if (found) {
        printf("FAILED: search should not have found pattern\n");
        pattern_free(pattern);
        matchlist_free(matches);
        return 0;
    }

    if (matches->count != 0) {
        printf("FAILED: expected 0 matches, got %zu\n", matches->count);
        pattern_free(pattern);
        matchlist_free(matches);
        return 0;
    }

    pattern_free(pattern);
    matchlist_free(matches);
    printf("PASSED: test_pattern_not_found\n");
    return 1;
}

int test_pattern_overlapping(void) {
    Pattern* pattern = pattern_create("aa", 0, 0);
    if (!pattern) {
        printf("FAILED: pattern_create returned NULL\n");
        return 0;
    }

    const char* data = "aaa";
    MatchList* matches = matchlist_create();
    if (!matches) {
        pattern_free(pattern);
        printf("FAILED: matchlist_create returned NULL\n");
        return 0;
    }

    search_pattern_ascii(pattern, data, strlen(data), matches);

    if (matches->count != 2) {
        printf("FAILED: expected 2 overlapping matches, got %zu\n", matches->count);
        pattern_free(pattern);
        matchlist_free(matches);
        return 0;
    }

    pattern_free(pattern);
    matchlist_free(matches);
    printf("PASSED: test_pattern_overlapping\n");
    return 1;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    int passed = 0;
    int total = 0;

    total++;
    if (test_pattern_create()) passed++;

    total++;
    if (test_pattern_case_insensitive()) passed++;

    total++;
    if (test_pattern_regex()) passed++;

    total++;
    if (test_line_counting()) passed++;

    total++;
    if (test_line_number_at_position()) passed++;

    total++;
    if (test_matchlist_operations()) passed++;

    total++;
    if (test_pattern_not_found()) passed++;

    total++;
    if (test_pattern_overlapping()) passed++;

    printf("\n");
    printf("================================\n");
    printf("Unit Test Results: %d/%d passed\n", passed, total);
    printf("================================\n");

    return (passed == total) ? 0 : 1;
}