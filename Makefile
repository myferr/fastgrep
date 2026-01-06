# FASTgrep Makefile

CC = gcc
CFLAGS = -O3 -march=native -pthread -Wall -Wextra -std=c99
LDFLAGS = -pthread

# Optional SIMD flags
# Uncomment based on your CPU:
# CFLAGS += -msse4.2
# CFLAGS += -mavx2

# Enable all SIMD features by default if supported
CFLAGS += -msse4.2

# Detect AVX2 support and enable if available
AVX2_CHECK := $(shell echo | $(CC) -mavx2 -E - >/dev/null 2>&1 && echo 1 || echo 0)
ifeq ($(AVX2_CHECK),1)
    CFLAGS += -mavx2
endif

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Target binary name
TARGET = $(BIN_DIR)/fstgrep

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Main source
MAIN = main.c

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# Link the binary
$(TARGET): $(OBJECTS) main.c
	$(CC) $(CFLAGS) $(OBJECTS) main.c -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Build without optimizations for debugging
debug: CFLAGS = -g -O0 -pthread -Wall -Wextra -std=c99 -DDEBUG
debug: clean all

# Build with extra optimizations
fast: CFLAGS = -Ofast -march=native -pthread -Wall -Wextra -std=c99 -DNDEBUG
fast: clean all

# Build with AVX2 only
avx2: CFLAGS = -O3 -march=native -mavx2 -pthread -Wall -Wextra -std=c99
avx2: clean all

# Build with SSE4.2 only
sse42: CFLAGS = -O3 -march=native -msse4.2 -pthread -Wall -Wextra -std=c99
sse42: clean all

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete"

# Install the binary (Linux/macOS)
install: all
	install -m 0755 $(TARGET) /usr/local/bin/fstgrep
	@echo "Installed fstgrep to /usr/local/bin/"

# Uninstall the binary
uninstall:
	rm -f /usr/local/bin/fstgrep
	@echo "Uninstalled fstgrep from /usr/local/bin/"

# Run tests
test: all
	@echo "Running unit tests..."
	@make -C tests run
	@echo "Running integration tests..."
	@make -C tests integration

# Run integration tests only
integration: all
	@make -C tests integration

# Show help
help:
	@echo "FASTgrep Makefile targets:"
	@echo "  all          - Build the binary (default)"
	@echo "  debug        - Build with debug symbols"
	@echo "  fast         - Build with extra optimizations"
	@echo "  avx2         - Build with AVX2 optimizations"
	@echo "  sse42        - Build with SSE4.2 optimizations"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install binary to /usr/local/bin"
	@echo "  uninstall    - Remove binary from /usr/local/bin"
	@echo "  test         - Run all tests"
	@echo "  integration  - Run integration tests only"
	@echo "  help         - Show this help message"

# Phony targets
.PHONY: all directories clean debug fast avx2 sse42 install uninstall test integration help