#ifndef STR_UTILS_H
#define STR_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

/**
 * @struct StringView
 * @brief An immutable reference to a string (does not guarantee null-termination).
 * Ideal for high-performance parsing without data duplication.
 * 
 * Example:
 * StringView sv = sv_from_parts("Hello World", 5); // Represents "Hello"
 */
typedef struct {
    const char *data;
    size_t len;
} StringView;

/**
 * @struct StaticBuilder
 * @brief A fixed-size buffer for safe string construction.
 * 
 * Example:
 * char buf[64];
 * StaticBuilder sb = sb_init(buf, sizeof(buf));
 */
typedef struct {
    char *data;
    size_t capacity;
    size_t len;
} StaticBuilder;

/* --- StringView Manipulation --- */

/**
 * @brief Creates a StringView from a pointer and a length.
 * Example: sv_from_parts("abc", 2) -> "ab"
 */
StringView sv_from_parts(const char *data, size_t len);

/**
 * @brief Creates a StringView from a null-terminated string.
 * Example: sv_from_cstr("hello")
 */
StringView sv_from_cstr(const char *s);

/**
 * @brief Compares two StringViews for equality.
 * Example: sv_equals(sv_from_cstr("a"), sv_from_cstr("a")) -> true
 */
bool sv_equals(StringView a, StringView b);

/**
 * @brief Compares a StringView with a null-terminated string.
 * Example: sv_equals_cstr(sv, "admin")
 */
bool sv_equals_cstr(StringView a, const char *b);

/**
 * @brief Removes leading and trailing whitespace.
 * Example: sv_trim(sv_from_cstr("  text  ")) -> "text"
 */
StringView sv_trim(StringView sv);

/**
 * @brief Splits a StringView by a delimiter, updating the input to the remainder.
 * Example:
 * StringView in = sv_from_cstr("a,b,c");
 * StringView part = sv_split_next(&in, ','); // returns "a", 'in' becomes "b,c"
 */
StringView sv_split_next(StringView *input, char delim);

/**
 * @brief Calculates a 32-bit FNV-1a hash of the StringView.
 * Example: uint32_t h = sv_hash(sv_from_cstr("key"));
 */
uint32_t sv_hash(StringView sv);

/* --- Strict Numerical Conversion (Supports 0x, 0b and _) --- */

/**
 * @brief Converts StringView to integer types. Returns false on overflow or invalid chars.
 * Supports hex (0x), binary (0b), and underscores as separators.
 * Example: 
 * int64_t val;
 * sv_to_int64(sv_from_cstr("0xFF_FF"), &val); // val = 65535
 */
bool sv_to_int64(StringView sv, int64_t *res);
bool sv_to_int32(StringView sv, int32_t *res);
bool sv_to_int16(StringView sv, int16_t *res);
bool sv_to_int8(StringView sv, int8_t *res);

bool sv_to_uint64(StringView sv, uint64_t *res);
bool sv_to_uint32(StringView sv, uint32_t *res);
bool sv_to_uint16(StringView sv, uint16_t *res);
bool sv_to_uint8(StringView sv, uint8_t *res);

/**
 * @brief Converts StringView to floating point.
 * Example: float f; sv_to_float32(sv_from_cstr("3.14"), &f);
 */
bool sv_to_float64(StringView sv, double *res);
bool sv_to_float32(StringView sv, float *res);

/* --- Hex and Protocol Utilities --- */

/**
 * @brief Converts a 1-2 char hex StringView to uint8_t.
 * Example: uint8_t val; sv_hex_to_uint8(sv_from_cstr("AF"), &val); // val = 175
 */
bool sv_hex_to_uint8(StringView sv, uint8_t *res);

/**
 * @brief Converts a long hex string to a byte array.
 * Example: uint8_t bytes[2]; size_t s = 2; sv_hex_to_uint8_array(sv_from_cstr("AABB"), bytes, &s);
 */
bool sv_hex_to_uint8_array(StringView sv, uint8_t *array, size_t *array_size);

/**
 * @brief Parses an IPv4 string into 4 bytes.
 * Example: uint8_t ip[4]; sv_parse_ipv4(sv_from_cstr("127.0.0.1"), ip);
 */
bool sv_parse_ipv4(StringView sv, uint8_t ip[4]);

/**
 * @brief Parses a MAC address string (supports ':', '-' or '.' as separators).
 * Example: uint8_t mac[6]; sv_parse_mac(sv_from_cstr("AA:BB:CC:DD:EE:FF"), mac);
 */
bool sv_parse_mac(StringView sv, uint8_t mac[6]);

/* --- Shell Parsing --- */

/**
 * @brief Parses a string into arguments, supporting double quotes for spaces.
 * Example: int argc = shell_parse_line("cmd \"arg with space\" 123", argv, 10);
 */
int shell_parse_line(char *line, StringView argv[], int max_args);

/* --- StaticBuilder --- */

/**
 * @brief Initializes a StaticBuilder using an existing external buffer.
 * Example: 
 *     char buffer[128]; 
 *     StaticBuilder sb = sb_init(buffer, sizeof(buffer));
 */
StaticBuilder sb_init(char *external_buffer, size_t size);

/**
 * @brief Resets the builder length to zero. The underlying buffer is reused.
 * Example: sb_reset(&sb);
 */
void sb_reset(StaticBuilder *sb);

/**
 * @brief Appends a StringView to the builder. Returns false if capacity is exceeded.
 * Example: sb_append_sv(&sb, sv_from_cstr("Data"));
 */
bool sb_append_sv(StaticBuilder *sb, StringView sv);

/**
 * @brief Appends a null-terminated string to the builder.
 * Example: sb_append_cstr(&sb, "Hello World");
 */
bool sb_append_cstr(StaticBuilder *sb, const char *s);

/**
 * @brief Appends a formatted string to the builder (printf-style).
 * Example: sb_append_fmt(&sb, "ID: %04d", 123);
 */
bool sb_append_fmt(StaticBuilder *sb, const char *fmt, ...);

/**
 * @brief Returns a StringView of the content currently stored in the builder.
 * Example: StringView result = sb_to_view(&sb);
 */
StringView sb_to_view(const StaticBuilder *sb);

/**
 * @brief Matches a StringView against a simple regular expression.
 * Supports '.', '*', '^', '$'.
 * - '.' matches any single character.
 * - '*' matches zero or more occurrences of the preceding character.
 * - '^' matches the beginning of the text.
 * - '$' matches the end of the text.
 * 
 * @warning This engine is recursive. Avoid using extremely long or complex
 * patterns on systems with limited stack space to prevent stack starvation.
 * 
 * Example:
 * sv_match(sv_from_cstr("a.c"), sv_from_cstr("abc")) -> true
 * sv_match(sv_from_cstr("a*b"), sv_from_cstr("aaab")) -> true
 * sv_match(sv_from_cstr("^abc"), sv_from_cstr("abcdef")) -> true
 * sv_match(sv_from_cstr("def$"), sv_from_cstr("abcdef")) -> true
 */
bool sv_match(StringView pattern, StringView text);

/**
 * @brief Macros for printing StringViews using printf-family functions.
 * PRIsv provides the format string "%.*s", and EXsv expands the structure into 
 * the required (length, data) arguments.
 * Example:
 *     StringView sv = sv_from_parts("BufferContent", 6);
 *     printf("The result is: " PRIsv "\n", EXsv(sv)); // Prints: The result is: Buffer
 */
#define PRIsv "%.*s"
#define EXsv(sv) (int)(sv).len, (sv).data

#endif // STR_UTILS_H