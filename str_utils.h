#ifndef STR_UTILS_H
#define STR_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h> // IWYU pragma: keep.
#include <stdarg.h>
#include <stdio.h>

/*
 * An immutable reference to a string (does not guarantee null-termination).
 * Ideal for high-performance parsing without data duplication.
 *
 * Example:
 *   StringView sv = sv_from_parts("Hello World", 5); // Represents "Hello"
 */
typedef struct {
    const char *data;
    size_t len;
} StringView;

/*
 * A fixed-size buffer for safe string construction.
 *
 * Example:
 *   char buf[64];
 *   StaticBuilder sb = sb_init(buf, sizeof(buf));
 */
typedef struct {
    char *data;
    size_t capacity;
    size_t len;
} StaticBuilder;

/*
 * A lock-free circular buffer for single-producer / single-consumer use.
 * 'head' is owned exclusively by the writer, 'tail' by the reader.
 * No 'len' field exists — occupancy is derived from head and tail alone,
 * which makes concurrent access safe without a mutex on any architecture
 * where size_t reads and writes are atomic (e.g. Cortex-M).
 * One slot is permanently reserved as a sentinel to distinguish full from empty,
 * so the usable capacity is always (size - 1) bytes.
 *
 * Example:
 *   char raw[9];                          // 9-byte buffer, 8 usable
 *   ByteStream bs = BYTESTREAM_INIT(raw, sizeof(raw));
 */
typedef struct {
    char   *buf;
    size_t  capacity; // Usable bytes (physical buffer size minus 1)
    size_t  head;     // Next write position (writer-owned)
    size_t  tail;     // Next read position  (reader-owned)
} ByteStream;


/* --- StringView Manipulation --- */

/*
 * Creates a StringView referencing 'len' bytes starting at 'data'.
 * The data is not copied and does not need to be null-terminated.
 * Use this to create a view into the middle of an existing buffer.
 *
 * Example:
 *   const char *buf = "Hello, World!";
 *   StringView sv = sv_from_parts(buf + 7, 5); // sv == "World"
 */
StringView sv_from_parts(const char *data, size_t len);

/*
 * Creates a StringView from a null-terminated C-string.
 * If 's' is NULL, returns an empty view (data=NULL, len=0).
 *
 * Example:
 *   StringView sv    = sv_from_cstr("hello"); // sv.len == 5;
 *   StringView empty = sv_from_cstr(NULL);    // sv.len == 0
 */
StringView sv_from_cstr(const char *s);

/*
 * Copies the contents of 'sv' into 'buffer' and appends a null terminator,
 * producing a valid C-string. The buffer must be at least sv.len + 1 bytes.
 * Returns 'buffer' on success, or NULL if 'buffer' is NULL or 'buf_len' is
 * too small to hold sv.len bytes plus the null terminator.
 * The original StringView is not modified.
 *
 * Example:
 *   char buf[16];
 *   StringView sv = sv_from_parts("hello", 5);
 *   char *s = sv_to_cstr(sv, buf, sizeof(buf)); // s == "hello", buf[5] == '\0';
 *   sv_to_cstr(sv, buf, 5);                     // NULL, buf_len too small (need 6)
 */
char *sv_to_cstr(StringView sv, char *buffer, size_t buf_len);

/*
 * Returns true if two StringViews have identical length and contents.
 * Comparison is binary-safe (works with embedded null bytes).
 *
 * Example:
 *   sv_equals(sv_from_cstr("abc"), sv_from_cstr("abc")); // true;
 *   sv_equals(sv_from_cstr("abc"), sv_from_cstr("ABC")); // false
 */
bool sv_equals(StringView a, StringView b);

/*
 * Returns true if a StringView equals a null-terminated C-string.
 * Returns false if 'b' is NULL.
 *
 * Example:
 *   StringView cmd = sv_from_cstr("reboot");
 *   if (sv_equals_cstr(cmd, "reboot")) { ... }
 */
bool sv_equals_cstr(StringView a, const char *b);

/*
 * Returns a new StringView with leading and trailing whitespace removed.
 * Does not modify the original buffer; only adjusts the pointer and length.
 *
 * Example:
 *   StringView sv = sv_trim(sv_from_cstr("  hello  ")); // sv == "hello"
 */
StringView sv_trim(StringView sv);

/*
 * Returns a sub-view of 'sv' starting at byte offset 'start' with at most 'length' bytes.
 * If 'start' is beyond sv.len, returns an empty view.
 * If 'start + length' exceeds sv.len, the view is clamped to the end.
 *
 * Example:
 *   StringView sv = sv_from_cstr("Hello, World!");
 *   sv_substr(sv, 7, 5); // "World"
 *   sv_substr(sv, 7, 99); // "World!" (clamped)
 *   sv_substr(sv, 99, 5); // "" (empty, start out of range)
 */
StringView sv_substr(StringView sv, size_t start, size_t length);

/*
 * Extracts the next token from '*input', delimited by 'delim', and advances
 * '*input' past the delimiter so successive calls iterate all tokens.
 * If 'delim' is not found, the entire remaining view is returned and
 * input->len is set to 0, ending the iteration.
 *
 * Example:
 *   StringView in = sv_from_cstr("one,two,three");
 *   while (in.len > 0) {
 *       StringView tok = sv_split_next(&in, ',');
 *       // tok: "one", then "two", then "three"
 *   }
 */
StringView sv_split_next(StringView *input, char delim);

/*
 * Computes a 32-bit FNV-1a hash of the StringView contents.
 * Suitable for use as a hash table key. Produces good distribution for
 * short-to-medium strings with very low code overhead.
 *
 * Example:
 *   uint32_t h = sv_hash(sv_from_cstr("sensor_temp")); // use as map key
 */
uint32_t sv_hash(StringView sv);

/* --- Strict Numerical Conversion (Supports 0x, 0b and _) --- */

/*
 * Parses a StringView into an int64_t. Returns false on parse or range error.
 * Supported formats: decimal ("42", "-7"), hex ("0xFF"), binary ("0b1010"),
 * underscore separators ("1_000", "0xFF_FF").
 *
 * Example:
 *   int64_t v;
 *   sv_to_int64(sv_from_cstr("-0x8000"), &v); // v = -32768
 */
bool sv_to_int64(StringView sv, int64_t *res);

/*
 * Parses a StringView into an int32_t. Returns false on parse or range error.
 *
 * Example:
 *   int32_t v;
 *   sv_to_int32(sv_from_cstr("-2_000"), &v); // v = -2000;
 *   sv_to_int32(sv_from_cstr("0xDEAD"),  &v); // v = 57005
 */
bool sv_to_int32(StringView sv, int32_t *res);

/*
 * Parses a StringView into an int16_t. Returns false on parse or range error.
 *
 * Example:
 *   int16_t v;
 *   sv_to_int16(sv_from_cstr("0b0111111111111111"), &v); // v = 32767
 */
bool sv_to_int16(StringView sv, int16_t *res);

/*
 * Parses a StringView into an int8_t. Returns false on parse or range error.
 *
 * Example:
 *   int8_t v;
 *   sv_to_int8(sv_from_cstr("-128"), &v); // v = -128;
 *   sv_to_int8(sv_from_cstr("200"),  &v); // false, overflow
 */
bool sv_to_int8(StringView sv, int8_t *res);

/*
 * Parses a StringView into a uint64_t. Rejects negative values (leading '-').
 * Returns false on parse or range error.
 *
 * Example:
 *   uint64_t v;
 *   sv_to_uint64(sv_from_cstr("0xFFFFFFFFFFFFFFFF"), &v); // v = UINT64_MAX
 */
bool sv_to_uint64(StringView sv, uint64_t *res);

/*
 * Parses a StringView into a uint32_t. Rejects negative values (leading '-').
 * Returns false on parse or range error.
 *
 * Example:
 *   uint32_t v;
 *   sv_to_uint32(sv_from_cstr("1_000"), &v); // v = 1000
 */
bool sv_to_uint32(StringView sv, uint32_t *res);

/*
 * Parses a StringView into a uint16_t. Rejects negative values (leading '-').
 * Returns false on parse or range error.
 *
 * Example:
 *   uint16_t v;
 *   sv_to_uint16(sv_from_cstr("0x1F4"), &v); // v = 500
 */
bool sv_to_uint16(StringView sv, uint16_t *res);

/*
 * Parses a StringView into a uint8_t. Rejects negative values (leading '-').
 * Returns false on parse or range error.
 *
 * Example:
 *   uint8_t v;
 *   sv_to_uint8(sv_from_cstr("255"), &v); // v = 255;
 *   sv_to_uint8(sv_from_cstr("256"), &v); // false, overflow
 */
bool sv_to_uint8(StringView sv, uint8_t *res);

/*
 * Parses a decimal floating-point number from a StringView into a double.
 * Supports an optional leading sign, a single decimal point, and '_' as a
 * digit separator (e.g. "1_000.50"). Does not support scientific notation.
 * Returns false on empty input, multiple decimal points, or non-digit characters.
 *
 * Example:
 *   double d;
 *   sv_to_float64(sv_from_cstr("3.14159"), &d); // d = 3.14159;
 *   sv_to_float64(sv_from_cstr("1.2.3"),   &d); // false, two dots
 */
bool sv_to_float64(StringView sv, double *res);

/*
 * Parses a decimal floating-point number from a StringView into a float.
 * Delegates to sv_to_float64, then checks the value fits within ±FLT_MAX.
 *
 * Example:
 *   float f;
 *   sv_to_float32(sv_from_cstr("-1_024.5"), &f); // f = -1024.5;
 *   sv_to_float32(sv_from_cstr("1e99"),     &f); // false, scientific notation unsupported
 */
bool sv_to_float32(StringView sv, float *res);

/* --- Hex and Protocol Utilities --- */

/*
 * Converts a 1 or 2 character hex string to a uint8_t.
 * Accepts both uppercase and lowercase hex digits.
 * Returns false if the input is empty, longer than 2 chars, or contains a non-hex character.
 *
 * Example:
 *   uint8_t v;
 *   sv_hex_to_uint8(sv_from_cstr("AF"), &v); // v = 0xAF (175);
 *   sv_hex_to_uint8(sv_from_cstr("0f"), &v); // v = 0x0F (15);
 *   sv_hex_to_uint8(sv_from_cstr("GG"), &v); // false, invalid digit
 */
bool sv_hex_to_uint8(StringView sv, uint8_t *res);

/*
 * Converts a hex string of even length into a byte array.
 * Each consecutive pair of hex characters is decoded into one byte.
 * On entry, '*array_size' must hold the capacity of 'array'.
 * On success, '*array_size' is updated to the number of bytes written.
 * Returns false if the input length is odd, the buffer is too small,
 * 'array' or 'array_size' is NULL, or any character is not a valid hex digit.
 *
 * Example:
 *   uint8_t buf[4]; size_t sz = sizeof(buf);
 *   sv_hex_to_uint8_array(sv_from_cstr("DeAdBeEf"), buf, &sz);
 *   // buf = {0xDE, 0xAD, 0xBE, 0xEF}, sz = 4
 */
bool sv_hex_to_uint8_array(StringView sv, uint8_t *array, size_t *array_size);

/*
 * Parses a dotted-decimal IPv4 address into a 4-byte array.
 * Expects exactly 4 decimal octets in range [0, 255] separated by '.'.
 * Returns false if any octet is out of range, non-numeric, or if the
 * number of octets is not exactly 4.
 *
 * Example:
 *   uint8_t ip[4];
 *   sv_parse_ipv4(sv_from_cstr("192.168.1.100"), ip); // ip = {192, 168, 1, 100}
 *   sv_parse_ipv4(sv_from_cstr("999.0.0.1"),     ip); // false, octet out of range
 */
bool sv_parse_ipv4(StringView sv, uint8_t ip[4]);

/*
 * Parses a MAC address string into a 6-byte array.
 * The input must be exactly 17 characters: 6 pairs of hex digits separated by
 * ':' or '-' (delimiter is detected from position 2 and must be used consistently).
 * Returns false on incorrect length, invalid or mixed delimiters, or invalid hex digits.
 *
 * Example:
 *   uint8_t mac[6];
 *   sv_parse_mac(sv_from_cstr("AA:BB:CC:DD:EE:FF"), mac); // {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};
 *   sv_parse_mac(sv_from_cstr("aa-bb-cc-dd-ee-ff"), mac); // also valid;
 *   sv_parse_mac(sv_from_cstr("AA:BB-CC:DD:EE:FF"), mac); // false, mixed delimiters
 */
bool sv_parse_mac(StringView sv, uint8_t mac[6]);

/* --- ByteStream --- */


/*
 * Static-initializer macro for global or static ByteStream variables.
 * 'size' is the total size of the backing buffer; usable capacity is size-1.
 *
 * Example:
 *   static char raw[65];
 *   static ByteStream bs = BYTESTREAM_INIT(raw, sizeof(raw)); // 64 usable bytes
 */
#define BYTESTREAM_INIT(buf, size) { (buf), (size) - 1, 0, 0 }

/*
 * Initializes a ByteStream at runtime over an existing buffer.
 * Equivalent to the BYTESTREAM_INIT macro but usable inside functions.
 *
 * Example:
 *   char raw[9];
 *   ByteStream bs = bs_init(raw, sizeof(raw)); // 8 usable bytes
 */
ByteStream bs_init(char *buf, size_t size);

/*
 * Resets the stream to empty by moving both cursors back to zero.
 * The buffer contents are not cleared; any unread data is silently discarded.
 *
 * Example:
 *   bs_write(&bs, "hello", 5);
 *   bs_reset(&bs); // stream is empty again
 */
void bs_reset(ByteStream *bs);

/*
 * Returns the number of bytes currently available to read.
 *
 * Example:
 *   bs_write(&bs, "hi", 2);
 *   bs_readable(&bs); // 2
 */
size_t bs_readable(const ByteStream *bs);

/*
 * Returns the number of bytes that can still be written before the stream is full.
 *
 * Example:
 *   // With an 8-byte usable capacity and 2 bytes written:
 *   bs_writable(&bs); // 6
 */
size_t bs_writable(const ByteStream *bs);

/*
 * Writes up to 'len' bytes from 'data' into the stream.
 * If fewer than 'len' bytes are available, only the fitting bytes are written.
 * Returns the number of bytes actually written.
 *
 * Example:
 *   char raw[5]; ByteStream bs = bs_init(raw, sizeof(raw)); // 4 usable
 *   bs_write(&bs, "hello", 5); // returns 4, last byte dropped
 */
size_t bs_write(ByteStream *bs, const char *data, size_t len);

/*
 * Copies up to 'len' bytes from the stream into 'dst' without consuming them.
 * Successive peeks return the same data until a bs_read advances the tail.
 * Returns the number of bytes copied.
 *
 * Example:
 *   char tmp[4];
 *   bs_peek(&bs, tmp, 4); // reads 4 bytes, stream position unchanged
 *   bs_peek(&bs, tmp, 4); // returns the same 4 bytes again
 */
size_t bs_peek(const ByteStream *bs, char *dst, size_t len);

/*
 * Copies up to 'len' bytes from the stream into 'dst' and advances the read cursor.
 * Returns the number of bytes consumed.
 *
 * Example:
 *   bs_write(&bs, "hello", 5);
 *   char out[3];
 *   bs_read(&bs, out, 3); // out="hel", 2 bytes remain in stream
 *   bs_read(&bs, out, 3); // out="lo",  returns 2
 */
size_t bs_read(ByteStream *bs, char *dst, size_t len);

/* --- Shell Parsing --- */

/*
 * Splits a command line string into an array of StringView tokens.
 * Tokens are separated by whitespace. Arguments enclosed in double quotes
 * are treated as a single token, allowing spaces inside them.
 * Does not modify the input buffer (zero-copy). Escaped quotes are not supported.
 * Returns the number of arguments parsed (argc).
 *
 * Example:
 *   StringView argv[8];
 *   int argc = shell_parse_line(sv_from_cstr("set name \"John Doe\""), argv, 8);
 *   // argc=3, argv[0]="set", argv[1]="name", argv[2]="John Doe"
 */
int shell_parse_line(StringView line, StringView argv[], int max_args);

/* --- StaticBuilder --- */

/*
 * Initializes a StaticBuilder over an existing external buffer.
 * The buffer is immediately null-terminated. All subsequent append operations
 * maintain null-termination, so 'sb.data' is always safe to pass to C-string APIs.
 *
 * Example:
 *   char buf[64];
 *   StaticBuilder sb = sb_init(buf, sizeof(buf));
 */
StaticBuilder sb_init(char *external_buffer, size_t size);

/*
 * Resets the builder to empty without freeing or reallocating the buffer.
 * Sets len to 0 and writes '\0' at buf[0], making it an empty C-string again.
 *
 * Example:
 *   sb_append_cstr(&sb, "temporary");
 *   sb_reset(&sb); // sb is empty again, buffer is reused
 */
void sb_reset(StaticBuilder *sb);

/*
 * Appends the contents of a StringView to the builder.
 * Returns false without modifying the builder if there is insufficient
 * remaining capacity (including space for the null terminator).
 *
 * Example:
 *   sb_append_sv(&sb, sv_from_cstr("hello"));
 *   sb_append_sv(&sb, sv_from_parts(", world", 7));
 *   // sb.data == "hello, world"
 */
bool sb_append_sv(StaticBuilder *sb, StringView sv);

/*
 * Appends a null-terminated C-string to the builder.
 * Equivalent to sb_append_sv(sb, sv_from_cstr(s)).
 *
 * Example:
 *   sb_append_cstr(&sb, "Status: ");
 *   sb_append_cstr(&sb, "OK");
 *   // sb.data == "Status: OK"
 */
bool sb_append_cstr(StaticBuilder *sb, const char *s);

/*
 * Appends a char to the builder.
 *
 * Example:
 *   sb_append_cstr(&sb, "Status: ");
 *   sb_append_char(&sb, '#');
 *   // sb.data == "Status: #"
 */
bool sb_append_char(StaticBuilder *sb, char c);
/*
 * Appends a printf-style formatted string to the builder.
 * Returns false if the formatted output would exceed the remaining capacity
 * or if a formatting error occurs. The builder is not modified on failure.
 *
 * Example:
 *   sb_append_fmt(&sb, "Temp: %.1f C", 36.6f);  // appends "Temp: 36.6 C";
 *   sb_append_fmt(&sb, "ID: %04d", 7);          // appends "ID: 0007"
 */
bool sb_append_fmt(StaticBuilder *sb, const char *fmt, ...);

/*
 * Returns a StringView over the builder's current content.
 * The view shares the builder's buffer and is valid as long as the builder
 * is not reset or its buffer is not reused.
 *
 * Example:
 *   sb_append_cstr(&sb, "hello");
 *   StringView sv = sb_to_view(&sb);
 *   printf("%" PRIsv "\n", EXsv(sv)); // prints: hello
 */
StringView sb_to_view(const StaticBuilder *sb);

/*
 * Matches a StringView against a simple regular expression (Rob Pike's engine).
 *
 * Supported metacharacters:
 *   '.'  — matches any single character.
 *   '*'  — matches zero or more occurrences of the preceding character.
 *   '+'  — matches one or more occurrences of the preceding character.
 *   '^'  — anchors the match to the start of the text.
 *   '$'  — anchors the match to the end of the text.
 *
 * Without '^', the pattern is searched across all positions in the text.
 * Character classes ([a-z]), '?', and escaped metacharacters are not supported.
 *
 * Example:
 *   sv_match(sv_from_cstr("a.c"),    sv_from_cstr("abc"));     // true  ('.' matches 'b');
 *   sv_match(sv_from_cstr("a*b"),    sv_from_cstr("aaab"));    // true  (three 'a's then 'b');
 *   sv_match(sv_from_cstr("^hello"), sv_from_cstr("hello!"));  // true  (anchored at start);
 *   sv_match(sv_from_cstr("end$"),   sv_from_cstr("the end")); // true  (anchored at end);
 *   sv_match(sv_from_cstr("^x"),     sv_from_cstr("abc"));     // false (no match at start)
 */
bool sv_match(StringView pattern, StringView text);

/*
 * Macros for printing StringViews using printf-family functions.
 * PRIsv provides the format specifier, EXsv expands the struct into the required arguments.
 *
 * Example:
 *   StringView sv = sv_from_parts("BufferContent", 6);
 *   printf("Result: %" PRIsv "\n", EXsv(sv)); // prints: Result: Buffer
 */
#define PRIsv ".*s"
#define EXsv(sv) (int)(sv).len, (sv).data

#endif // STR_UTILS_H
