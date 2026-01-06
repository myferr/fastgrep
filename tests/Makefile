# Tests Makefile

CC = gcc
CFLAGS = -O3 -march=native -pthread -Wall -Wextra -std=c99 -I../include
LDFLAGS = -pthread

# Build directory
BUILD_DIR = ../build
BIN_DIR = ../bin

# Source files
SRC_DIR = ../src

# Object files (reuse main build)
OBJECTS = $(BUILD_DIR)/file_reader.o $(BUILD_DIR)/regex_simd.o \
          $(BUILD_DIR)/search.o $(BUILD_DIR)/output.o $(BUILD_DIR)/logger.o

# Test binaries
UNIT_TEST = $(BIN_DIR)/unit_tests

# Default target
all: directories $(UNIT_TEST)

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# Build unit tests
$(UNIT_TEST): unit_tests.c $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) unit_tests.c -o $@ $(LDFLAGS)
	@echo "Unit tests built: $@"

# Run unit tests
run: $(UNIT_TEST)
	@echo "Running unit tests..."
	@./$(UNIT_TEST)

# Integration tests
integration: $(UNIT_TEST)
	@echo "Running integration tests..."
	@echo "Creating test files..."
	@mkdir -p test_data
	@echo "hello world" > test_data/test1.txt
	@echo "hello universe" > test_data/test2.txt
	@echo "goodbye world" > test_data/test3.txt
	@echo "HELLO WORLD" > test_data/test4.txt
	@echo "error123" > test_data/test5.txt
	@echo "error456" > test_data/test6.txt
	@echo "no match here" > test_data/test7.txt
	@echo "multi\nline\nfile" > test_data/test8.txt

	@echo "Test 1: Basic search"
	@../bin/fgrep "hello" test_data/test1.txt
	@echo ""

	@echo "Test 2: Case insensitive search"
	@../bin/fgrep -i "hello" test_data/test4.txt
	@echo ""

	@echo "Test 3: Multiple files"
	@../bin/fgrep "hello" test_data/test1.txt test_data/test2.txt
	@echo ""

	@echo "Test 4: Regex search"
	@../bin/fgrep -e "error[0-9]+" test_data/test5.txt test_data/test6.txt
	@echo ""

	@echo "Test 5: Line numbers"
	@../bin/fgrep -n "line" test_data/test8.txt
	@echo ""

	@echo "Test 6: Recursive directory search"
	@../bin/fgrep -r "hello" test_data
	@echo ""

	@echo "Test 7: No matches"
	@../bin/fgrep "xyz" test_data/test1.txt || echo "No matches found (expected)"
	@echo ""

	@echo "Test 8: Color output (if TTY)"
	@../bin/fgrep --color "hello" test_data/test1.txt
	@echo ""

	@echo "Test 9: Quiet mode"
	@../bin/fgrep -q "hello" test_data/test1.txt && echo "Match found (exit code 0)" || echo "No match (exit code 1)"
	@echo ""

	@echo "Cleaning up test files..."
	@rm -rf test_data

	@echo "Integration tests complete!"

# Clean test artifacts
clean:
	rm -rf test_data

# Help
help:
	@echo "Tests Makefile targets:"
	@echo "  all          - Build all tests (default)"
	@echo "  run          - Run unit tests"
	@echo "  integration  - Run integration tests"
	@echo "  clean        - Remove test artifacts"
	@echo "  help         - Show this help message"

.PHONY: all run integration clean help