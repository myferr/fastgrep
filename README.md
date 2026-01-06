# fastgrep

A high-performance, dependency-free CLI grep replacement written in C. fastgrep is designed for maximum speed using memory-mapped I/O, SIMD-accelerated pattern matching, and multi-threaded search.

## Features

- **Ultra-fast file searching** - Memory-mapped files for large files, buffered reading for small files
- **SIMD acceleration** - SSE4.2 and AVX2 support for ASCII substring search
- **Multi-threaded** - POSIX threads for parallel file processing
- **Pattern matching** - Full regex support (POSIX extended) and optimized ASCII substring search
- **Case-insensitive search** - Optional case-insensitive matching
- **Recursive directory search** - Search entire directory trees
- **Rich output** - ANSI color highlighting, line numbers, file names
- **Cross-platform** - Linux, macOS, and Windows (where supported)
- **Zero dependencies** - Uses only C standard library and POSIX APIs

## Performance

fastgrep is optimized for raw performance:

- **Memory-mapped I/O** - Direct file access without copying data
- **SIMD instructions** - SSE4.2/AVX2 for fast pattern matching
- **Multi-threading** - Parallel search across multiple files
- **Minimal allocations** - Avoids memory allocations in hot loops
- **Precompiled regex** - Regex patterns compiled once and reused

## Building

### Prerequisites

- GCC or Clang compiler with C99 support
- POSIX-compliant system (Linux, macOS, BSD)
- Make utility

### Build Instructions

```bash
# Clone the repository
git clone <repository-url>
cd fastgrep

# Build with optimizations (default)
make

# Build with debug symbols
make debug

# Build with extra optimizations
make fast

# Build with AVX2 only
make avx2

# Build with SSE4.2 only
make sse42

# Install to /usr/local/bin
sudo make install

# Uninstall
sudo make uninstall
```

The binary will be created at `bin/fgrep`.

### Build Flags

The Makefile uses the following optimization flags by default:
- `-O3` - Maximum optimization
- `-march=native` - Optimize for your CPU
- `-pthread` - POSIX threads
- `-msse4.2` - Enable SSE4.2 instructions
- `-mavx2` - Enable AVX2 instructions (if supported)

## Usage

### Basic Usage

```bash
# Search for a pattern in a single file
fgrep pattern file.txt

# Search in multiple files
fgrep pattern file1.txt file2.txt file3.txt

# Search recursively in a directory
fgrep -r pattern /path/to/directory

# Case-insensitive search
fgrep -i pattern file.txt

# Show line numbers
fgrep -n pattern file.txt

# Use regex matching
fgrep -e "error[0-9]+" file.txt

# Multi-threaded search
fgrep --threads 4 pattern *.log
```

### Command-Line Options

```
Pattern Matching:
  -e, --regex            Use regex matching (default: ASCII substring)
  -i, --ignore-case      Case-insensitive search

Search Options:
  -r, --recursive        Recursively search directories
      --threads <N>      Number of threads (default: 1)

Output Options:
  -n, --line-number      Show line numbers
      --no-line-number   Don't show line numbers
      --color            Highlight matches (default when TTY)
      --no-color         Don't highlight matches
  -q, --quiet            Quiet mode (only exit code matters)

Other Options:
  -v, --verbose          Verbose output
  -h, --help             Show help message
      --version          Show version information
```

### Examples

```bash
# Find all occurrences of "error" in log files
fgrep error *.log

# Find case-insensitive "hello" recursively
fgrep -i -r hello /path/to/code

# Use regex to find error codes
fgrep -e "error[0-9]+" application.log

# Search with line numbers and colors
fgrep -n --color "TODO" src/

# Multi-threaded search across many files
fgrep --threads 8 pattern /path/to/large/directory

# Quiet mode for scripts (exit code only)
fgrep -q "critical" system.log && echo "Found!"
```

## Architecture

fastgrep uses a modular architecture:

### Source Files

- **main.c** - Entry point, argument parsing, orchestration
- **file_reader.c** - Memory-mapped and buffered file I/O, directory traversal
- **regex_simd.c** - SIMD-accelerated pattern matching and regex support
- **search.c** - Multi-threaded search logic and task queue management
- **output.c** - Output formatting, colors, line numbers, file names
- **logger.c** - Debug and performance logging

### Header Files

- **include/file_reader.h** - File reading interfaces
- **include/regex_simd.h** - Pattern matching interfaces
- **include/search.h** - Search and threading interfaces
- **include/output.h** - Output formatting interfaces
- **include/logger.h** - Logging interfaces

## Testing

### Run Unit Tests

```bash
make test
```

### Run Integration Tests

```bash
make integration
```

### Run All Tests

```bash
make test
make integration
```

## Performance Notes

- **Memory-mapped files** are used for files larger than 1MB
- **SIMD acceleration** is automatically enabled for ASCII patterns on supported CPUs
- **Multi-threading** provides near-linear speedup for multiple files
- **Regex mode** is slower than ASCII substring search; use ASCII when possible

## Limitations

- Regex matching uses POSIX extended regex (not PCRE)
- SIMD acceleration only works for ASCII substring patterns
- Large files require sufficient virtual memory for memory mapping
- Windows support is limited to platforms with POSIX APIs

## License

See LICENSE file for details.

## Contributing

Contributions are welcome! Please ensure:
- Code follows the existing style
- All tests pass
- Performance optimizations are benchmarked
- Cross-platform compatibility is maintained

## Version History

- **1.0.0** - Initial release
  - Memory-mapped I/O
  - SIMD-accelerated search (SSE4.2/AVX2)
  - Multi-threaded processing
  - Full regex support
  - ANSI color output
  - Line numbers and file names