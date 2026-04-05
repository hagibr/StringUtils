# StringUtils

A high-performance, **zero-allocation** string utility library written in C, specifically designed for embedded systems and safety-critical applications.

## Purpose

The primary motivation of this project is to provide a robust alternative to standard C string manipulation functions (like `strtok`, `strtol`, or `sprintf`) which often require null-terminated strings, modify source buffers, or lack strict error handling. 

**Note**: To maintain a strictly zero-allocation environment, it is recommended to use a custom `printf` library such as eyalroz/printf, as many standard library implementations of `printf` and `vsnprintf` may internally perform heap allocations.

This library introduces two core primitives:
1.  **StringView**: An immutable reference to a string slice (pointer + length). It allows for zero-copy parsing of large buffers without needing to insert `\0` characters or duplicate data.
2.  **StaticBuilder**: A fixed-size buffer wrapper for safe string construction, preventing buffer overflows and ensuring consistent null-termination without dynamic memory allocation.

## Key Features

*   **No Dynamic Allocation**: No calls to `malloc`, `free`, or `realloc`. All memory is managed on the stack or provided via fixed-size buffers.
*   **Non-Destructive Parsing**: Unlike `strtok`, which modifies the source buffer, `StringView` operations are immutable.
*   **Thread-Safety**: The functions are reentrant and do not rely on global state.
*   **Strict Numerical Conversion**: Parsers for `int8` through `int64` (and unsigned equivalents) that support hexadecimal (`0x`), binary (`0b`), and underscores (`_`) as digit separators.
*   **Protocol Helpers**: Built-in support for parsing IPv4 addresses, MAC addresses, and Hexadecimal byte arrays.
*   **Shell Tokenizer**: A zero-copy line parser that supports quoted arguments, ideal for implementing UART-based Command Line Interfaces (CLIs).
*   **Fast Hashing**: Implementation of the 32-bit FNV-1a hash for quick command dispatching or hash table lookups.
*   **Simple Regex Matching**: A lightweight, recursive regular expression engine supporting `.` (any char), `*` (zero or more), `^` (start of string), and `$` (end of string). **Warning**: Recursive implementation; avoid complex patterns on small stacks.

## Core Components

### StringView
A `StringView` is a lightweight structure containing a `const char *data` and a `size_t len`. Because it tracks length explicitly, it can point to any segment of a larger buffer.

```c
StringView input = sv_from_cstr("GET /index.html HTTP/1.1");
StringView method = sv_split_next(&input, ' '); // method = "GET", input = "/index.html HTTP/1.1"
```

### StaticBuilder
`StaticBuilder` ensures you never write past the end of a buffer. It is a safer, stateful alternative to `strcat` or `snprintf` for building messages.

```c
char buf[128];
StaticBuilder sb = sb_init(buf, sizeof(buf));
sb_append_cstr(&sb, "IP: ");
sb_append_sv(&sb, ip_view);
```

## Embedded Systems Use Cases

*   **Command Parsing**: Implementing serial/telemetry CLIs using `shell_parse_line`.
*   **Protocol Decoding**: Extracting fields from NMEA (GPS), AT commands (Modems), or custom binary/text protocols.
*   **Memory Efficiency**: Processing large packets in-place without duplicating sub-strings.
*   **Pattern Validation**: Quickly checking if a string conforms to a simple pattern.
*   **Deterministic Latency**: By avoiding the heap, you eliminate the risk of heap fragmentation and non-deterministic allocation times.

## Getting Started

### Prerequisites
*   A C99 compatible compiler (GCC, Clang, IAR, Keil, etc.)
*   CMake (for building the test CLI)

### Building the Interactive Test CLI

The project includes an interactive CLI in `main.c` to test all library functions.

```bash
mkdir build
cd build
cmake ..
make
./StringUtils
```

### Example CLI Commands

```text
> help
> int32 0xFF_FF
> ipv4 192.168.1.1
> hash "my secret key"
> split "one:two:three" ":"
```

## License

This project is provided under the MIT License. Feel free to use it in your commercial or private embedded projects.

---
*Maintained by Alexandre - High-performance string utilities for restricted environments.*
*Made with the help of Gemini Code Assist.*