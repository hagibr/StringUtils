#include "str_utils.h"
#include <limits.h>
#include <float.h>

/* --- Basic StringView --- */

/**
 * sv_from_parts: Creates a view over an existing buffer slice (no copy, no null-terminator needed).
 * sv_from_cstr:  Creates a view from a null-terminated string. Accepts NULL safely (returns empty view).
 */
StringView sv_from_parts(const char *data, size_t len) {
    return (StringView){data, len};
}
StringView sv_from_cstr(const char *s) {
    return (StringView){s, s ? strlen(s) : 0};
}

/**
 * Compares two StringViews for equality. Uses memcmp for binary-safe, length-aware comparison.
 * Returns false immediately if lengths differ, avoiding unnecessary byte scanning.
 */
bool sv_equals(StringView a, StringView b) {
    return (a.len == b.len) && (memcmp(a.data, b.data, a.len) == 0);
}

/**
 * Compares a StringView against a C-string literal without wrapping it in a temporary StringView.
 * Returns false if 'b' is NULL.
 */
bool sv_equals_cstr(StringView a, const char *b) {
    if (!b) return false;
    size_t b_len = strlen(b);
    return (a.len == b_len) && (memcmp(a.data, b, a.len) == 0);
}

/**
 * Returns a new StringView with leading and trailing whitespace removed.
 * Does not modify the underlying buffer — only adjusts the data pointer and length (O(1) memory).
 */
StringView sv_trim(StringView sv) {
    if (sv.len == 0) return sv;
    // Left trimming
    while (sv.len > 0 && isspace((unsigned char)*sv.data)) {
        sv.data++; sv.len--;
    }
    // Right trimming
    while (sv.len > 0 && isspace((unsigned char)sv.data[sv.len - 1])) {
        sv.len--;
    }
    return sv;
}

/**
 * Extracts the next token from 'input' up to the first occurrence of 'delim', then advances
 * 'input' past the delimiter so the next call returns the following token.
 * If 'delim' is not found, returns the entire remaining view and sets input->len to 0.
 * Use in a loop: while (input.len > 0) { StringView tok = sv_split_next(&input, ','); ... }
 */
StringView sv_split_next(StringView *input, char delim) {
    if (input->len == 0) return (StringView){NULL, 0};
    const char *start = input->data;
    const char *end = memchr(start, delim, input->len);
    if (!end) {
        StringView token = *input;
        input->data += input->len; input->len = 0;
        return token;
    }
    size_t token_len = end - start;
    StringView token = {start, token_len};
    input->data = end + 1; input->len -= (token_len + 1);
    return token;
}

/**
 * Computes a 32-bit FNV-1a hash of the StringView contents.
 * Suitable for hash table keys. Good distribution for short-to-medium strings
 * with minimal code footprint — a good fit for embedded targets.
 */
uint32_t sv_hash(StringView sv) {
    uint32_t hash = 0x811c9dc5;
    for (size_t i = 0; i < sv.len; i++) {
        hash ^= (uint8_t)sv.data[i];
        hash *= 0x01000193;
    }
    return hash;
}

/* --- Internal Integer Conversion Engine --- */

/**
 * @brief Internal engine to convert a StringView to a 64-bit unsigned integer.
 * 
 * Supported Input Formats:
 * - Optional leading sign: '-' (sets neg flag) or '+'.
 * - Base detection: '0x'/'0X' for Hexadecimal, '0b'/'0B' for Binary. Default is Decimal.
 * - Separators: Underscores '_' are skipped (e.g., "0x12_34" or "1_000").
 * 
 * Early Exit / Failure Conditions (Returns false):
 * 1. Empty input: sv.len is 0.
 * 2. Incomplete input: String contains only a sign ("-") or only a prefix ("0x").
 * 3. Invalid characters: Any character that isn't a digit for the current base and isn't '_'.
 * 4. Digit out of range: For example, '2' in a binary string or 'A' in decimal.
 * 5. Overflow: The value exceeds UINT64_MAX during the calculation.
 * 6. Missing digits: If no valid digits are found (e.g., input is just underscores).
 */
static bool sv_internal_to_u64(StringView sv, uint64_t *result, bool *neg) {
    if (sv.len == 0) return false; // Exit 1: Empty input
    size_t i = 0; uint64_t val = 0; int base = 10; *neg = false;

    // Handle sign
    if (sv.data[i] == '-') { *neg = true; i++; }
    else if (sv.data[i] == '+') i++;

    // Detect base prefix
    if (i + 2 <= sv.len && sv.data[i] == '0') {
        char p = sv.data[i+1];
        if (p == 'x' || p == 'X') { base = 16; i += 2; }
        else if (p == 'b' || p == 'B') { base = 2; i += 2; }
    }
    if (i == sv.len) return false; // Exit 2: Input is just a sign or prefix

    bool has_digits = false;
    for (; i < sv.len; i++) {
        char c = sv.data[i];
        if (c == '_') continue; // Support for underscores as separators
        uint8_t digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (base == 16 && (c|32) >= 'a' && (c|32) <= 'f') digit = (c|32) - 'a' + 10;
        else return false; // Exit 3: Invalid character for current base

        if (digit >= (uint8_t)base) return false; // Exit 4: Digit exceeds base
        if (val > (UINT64_MAX / (uint64_t)base)) return false; // Exit 5: Potential overflow
        val = (val * base) + digit;
        has_digits = true;
    }
    if (!has_digits) return false; // Exit 6: No digits provided (e.g., "0x_")
    *result = val; return true;
}

/**
 * Public integer conversion wrappers. Each calls sv_internal_to_u64 and then
 * checks that the parsed value fits within the target type's range.
 * Returns false on overflow, negative value for unsigned types, or any parse error.
 */

bool sv_to_uint64(StringView sv, uint64_t *res) {
    uint64_t v;
    bool n;
    if (sv_internal_to_u64(sv, &v, &n) && !n) {
        *res = v;
        return true;
    }
    return false;
}

bool sv_to_uint32(StringView sv, uint32_t *res) {
    uint64_t v;
    bool n;
    if (sv_internal_to_u64(sv, &v, &n) && !n && v <= UINT32_MAX) {
        *res = (uint32_t)v;
        return true;
    }
    return false;
}

bool sv_to_uint16(StringView sv, uint16_t *res) {
    uint64_t v;
    bool n;
    if (sv_internal_to_u64(sv, &v, &n) && !n && v <= UINT16_MAX) {
        *res = (uint16_t)v;
        return true;
    }
    return false;
}

bool sv_to_uint8(StringView sv, uint8_t *res) {
    uint64_t v;
    bool n;
    if (sv_internal_to_u64(sv, &v, &n) && !n && v <= UINT8_MAX) {
        *res = (uint8_t)v;
        return true;
    }
    return false;
}

bool sv_to_int64(StringView sv, int64_t *res) {
    uint64_t v;
    bool n;
    if (!sv_internal_to_u64(sv, &v, &n)) {
        return false;
    }
    if (!n && v > (uint64_t)INT64_MAX) {
        return false;
    }
    if (n && v > (uint64_t)INT64_MAX + 1) {
        return false;
    }
    *res = n ? -(int64_t)v : (int64_t)v;
    return true;
}

bool sv_to_int32(StringView sv, int32_t *res) {
    int64_t v;
    if (sv_to_int64(sv, &v) && v <= INT32_MAX && v >= INT32_MIN) {
        *res = (int32_t)v;
        return true;
    }
    return false;
}

bool sv_to_int16(StringView sv, int16_t *res) {
    int64_t v;
    if (sv_to_int64(sv, &v) && v <= INT16_MAX && v >= INT16_MIN) {
        *res = (int16_t)v;
        return true;
    }
    return false;
}

bool sv_to_int8(StringView sv, int8_t *res) {
    int64_t v;
    if (sv_to_int64(sv, &v) && v <= INT8_MAX && v >= INT8_MIN) {
        *res = (int8_t)v;
        return true;
    }
    return false;
}

/* --- Strict Floating Point --- */

/**
 * Parses a decimal floating-point number from a StringView into a double.
 * Implemented manually because strtod/atof require a null-terminated string.
 * Supports optional leading sign, a single decimal point, and '_' as a digit separator.
 * Returns false on empty input, multiple decimal points, or non-digit characters.
 */
bool sv_to_float64(StringView sv, double *res) {
    if (sv.len == 0) {
        return false;
    }

    size_t i = 0;
    double val = 0.0;
    double sign = 1.0;

    // 1. Handle optional sign
    if (sv.data[i] == '-') {
        sign = -1.0;
        i++;
    } else if (sv.data[i] == '+') {
        i++;
    }

    bool has_digits = false;
    bool dot_seen = false;
    double weight = 0.1;

    // 2. Parse integer and fractional parts
    for (; i < sv.len; i++) {
        char c = sv.data[i];

        // Support for underscores as separators (e.g., 1_000.50)
        if (c == '_') {
            continue;
        }

        // Handle the decimal point; only one is allowed
        if (c == '.') {
            if (dot_seen) return false;
            dot_seen = true;
            continue;
        }

        if (!isdigit((unsigned char)c)) {
            return false;
        }

        if (!dot_seen) {
            // Integer part: shift existing value left and add digit
            val = val * 10.0 + (c - '0');
        } else {
            // Fractional part: add digit weighted by its position
            val += (c - '0') * weight;
            weight /= 10.0;
        }
        has_digits = true;
    }

    if (!has_digits) {
        return false;
    }

    *res = val * sign;
    return true;
}

/**
 * Parses a decimal floating-point number into a float.
 * Delegates to sv_to_float64 and then checks the value fits within float range (±FLT_MAX).
 * Returns false if the value would overflow a 32-bit float.
 */
bool sv_to_float32(StringView sv, float *res) {
    double v;
    if (sv_to_float64(sv, &v) && v <= FLT_MAX && v >= -FLT_MAX) {
        *res = (float)v;
        return true;
    }
    return false;
}

/* --- Hex and Protocols --- */

/**
 * Converts a 1 or 2 character hex string (e.g. "A3") to a uint8_t.
 * Accepts both uppercase and lowercase hex digits. Rejects inputs longer than 2 chars.
 */
bool sv_hex_to_uint8(StringView sv, uint8_t *res) {
    if (sv.len == 0 || sv.len > 2) return false;
    uint32_t val = 0;
    for (size_t i = 0; i < sv.len; i++) {
        char c = sv.data[i];
        val <<= 4;
        if (c >= '0' && c <= '9') {
            val |= (uint32_t)(c - '0');
        } else if ((c|32) >= 'a' && (c|32) <= 'f') {
            val |= (uint32_t)((c|32) - 'a' + 10);
        } else {
            return false; // Invalid hex digit
        }
    }
    *res = (uint8_t)val; return true;
}

/**
 * Converts a hex string of even length (e.g. "AABBCC") into a byte array.
 * Each pair of hex characters maps to one byte. Fails if the input length is odd,
 * if the output buffer is too small, or if any character is not a valid hex digit.
 * On success, updates *array_size to the number of bytes written.
 */
bool sv_hex_to_uint8_array(StringView sv, uint8_t *array, size_t *array_size) {
    if (!array || !array_size || sv.len % 2 != 0) return false;
    size_t needed = sv.len / 2;
    if (needed > *array_size) return false;
    for (size_t i = 0; i < needed; i++) {
        StringView b = {sv.data + (i * 2), 2};
        if (!sv_hex_to_uint8(b, &array[i])) return false;
    }
    *array_size = needed; return true;
}

/**
 * Parses a dotted-decimal IPv4 address (e.g. "192.168.1.1") into a 4-byte array.
 * Splits on '.' and converts each octet with sv_to_uint8. Returns false if any
 * octet is out of range, non-numeric, or if there are more/fewer than 4 octets.
 */
bool sv_parse_ipv4(StringView sv, uint8_t ip[4]) {
    StringView in = sv;
    for (int i = 0; i < 4; i++) {
        StringView p = sv_split_next(&in, '.');
        if (!sv_to_uint8(p, &ip[i])) return false;
    }
    return in.len == 0;
}

/**
 * Parses a MAC address string (e.g. "AA:BB:CC:DD:EE:FF" or "AA-BB-CC-DD-EE-FF") into
 * a 6-byte array. The delimiter is detected from position 2 and must be ':' or '-'
 * used consistently across all 5 separators. Returns false on any format violation.
 */
bool sv_parse_mac(StringView sv, uint8_t mac[6]) {
    if (sv.len != 17) return false;
    char delim = sv.data[2];
    if (delim != ':' && delim != '-') return false;

    StringView in = sv;
    for (int i = 0; i < 6; i++) {
        if (i < 5 && sv.data[2 + (i * 3)] != delim) return false;
        StringView p = sv_split_next(&in, (i < 5) ? delim : '\0');
        if (!sv_hex_to_uint8(p, &mac[i])) return false;
    }
    return in.len == 0;
}

/* --- Shell Parsing --- */

/**
 * Splits a command line string into an array of StringView tokens, respecting double-quoted
 * arguments (e.g. cmd "arg with spaces" 123 -> 3 tokens). Does not modify the input buffer.
 * Unlike strtok, it is re-entrant and zero-copy. Escaped quotes are not supported — doing so
 * would require either modifying the buffer or allocating memory.
 * Returns the number of parsed arguments (argc).
 */
int shell_parse_line(char *line, StringView argv[], int max_args) {
    StringView input = sv_trim(sv_from_cstr(line));
    int argc = 0;
    while (argc < max_args && input.len > 0) {
        if (input.data[0] == '"') {
            input.data++; input.len--; // Skip "
            StringView arg = sv_split_next(&input, '"');
            argv[argc++] = arg;
        } else {
            const char *start = input.data; size_t l = 0;
            while (l < input.len && !isspace((unsigned char)start[l])) l++;
            argv[argc++] = (StringView){start, l};
            input.data += l; input.len -= l;
        }
        input = sv_trim(input);
    }
    return argc;
}

/* --- StaticBuilder --- */

/**
 * Initializes a StaticBuilder over an existing external buffer.
 * The buffer is always kept null-terminated after every operation, making it
 * safe to pass to legacy C functions (e.g. printf("%s", sb.data)).
 */
StaticBuilder sb_init(char *buf, size_t size) {
    StaticBuilder sb = {buf, size, 0};
    if (size > 0) sb.data[0] = '\0';
    return sb;
}

/**
 * Resets the builder to empty without freeing or reallocating the buffer.
 * The first byte is set to '\0' so the buffer is immediately usable as an empty C-string.
 */
void sb_reset(StaticBuilder *sb) {
    sb->len = 0;
    if (sb->capacity > 0) sb->data[0] = '\0';
}

/**
 * Appends the contents of a StringView to the builder.
 * Returns false without modifying the builder if there is insufficient remaining capacity.
 */
bool sb_append_sv(StaticBuilder *sb, StringView sv) {
    if (sb->len + sv.len + 1 > sb->capacity) return false;
    memcpy(sb->data + sb->len, sv.data, sv.len);
    sb->len += sv.len; sb->data[sb->len] = '\0';
    return true;
}

/**
 * Appends a null-terminated C-string to the builder. Thin wrapper around sb_append_sv.
 */
bool sb_append_cstr(StaticBuilder *sb, const char *s) {
    return sb_append_sv(sb, sv_from_cstr(s));
}

/**
 * Appends a printf-style formatted string to the builder.
 * Returns false if the formatted output would exceed the remaining capacity (truncation is
 * detected via vsnprintf's return value) or if a formatting error occurs.
 */
bool sb_append_fmt(StaticBuilder *sb, const char *fmt, ...) {
    if (sb->len + 1 >= sb->capacity) return false;
    va_list args; va_start(args, fmt);
    int res = vsnprintf(sb->data + sb->len, sb->capacity - sb->len, fmt, args);
    va_end(args);
    if (res < 0 || (size_t)res >= (sb->capacity - sb->len)) return false;
    sb->len += (size_t)res; return true;
}

/**
 * Returns a StringView over the builder's current content. The view is valid as long
 * as the builder's buffer is not reallocated or reset.
 */
StringView sb_to_view(const StaticBuilder *sb) {
    return (StringView){sb->data, sb->len};
}

/* --- Regex Utilities (Rob Pike's implementation) --- */

/**
 * @brief Internal helper function to match a regular expression against a text,
 * assuming the match starts at the current 'text' position.
 * 
 * This is a recursive function that tries to match the current regex
 * character(s) against the current text character(s).
 * 
 * Early Exit / Failure Conditions:
 * - If regex is exhausted (len == 0), it's a match.
 * - If '*' quantifier is found, delegates to match_star.
 * - If '$' is found and regex is exhausted, matches if text is also exhausted.
 * - If text is exhausted but regex is not, it's a mismatch (unless regex is '$').
 * - If characters don't match (and not '.'), it's a mismatch.
 */
static bool match_here(StringView pattern, StringView text);

/**
 * @brief Internal helper function to handle the '*' quantifier.
 * Matches 'c' zero or more times, then attempts to match the rest of the regex.
 * 
 * It iteratively tries to match the rest of the regex (re + 2) against
 * the current text, then against the text after consuming 'c', and so on.
 * This is a backtracking approach.
 */
static bool match_star(char c, StringView pattern, StringView text) {
    do {
        // Try to match the rest of the regex (after 'c*') against the current text
        if (match_here(pattern, text)) return true;
        
        // If 'c' cannot be matched against the current text, or text is exhausted,
        // then 'c*' cannot consume more, so stop trying.
        if (text.len == 0 || (c != '.' && *text.data != c)) break;
        
        // Consume 'c' and try again
        text.data++;
        text.len--;
    } while (true);
    return false;
}

static bool match_here(StringView pattern, StringView text) {
    // If the regex is exhausted, we've found a match
    if (pattern.len == 0) return true;

    // If the next regex character is '*', handle it with match_star
    if (pattern.len >= 2 && pattern.data[1] == '*') {
        return match_star(pattern.data[0], (StringView){pattern.data + 2, pattern.len - 2}, text);
    }

    // If regex is '$' and it's the last character, match only if text is exhausted
    if (pattern.len == 1 && pattern.data[0] == '$') {
        return text.len == 0;
    }

    // If text is not exhausted and current regex char matches current text char (or regex char is '.')
    if (text.len > 0 && (pattern.data[0] == '.' || pattern.data[0] == *text.data)) {
        // Recursively match the rest of the regex against the rest of the text
        return match_here((StringView){pattern.data + 1, pattern.len - 1}, (StringView){text.data + 1, text.len - 1});
    }

    // No match found
    return false;
}

/**
 * @brief Public function to match a StringView against a simple regular expression.
 * 
 * Handles '^' for anchoring the match to the beginning of the text.
 * If no '^' is present, it iterates through the text, attempting to match the
 * regex at each possible starting position. This is a brute-force search.
 *
 * @warning This implementation is recursive. Users should be cautious with 
 * pattern complexity in stack-constrained embedded environments.
 */
bool sv_match(StringView pattern, StringView text) {
    if (pattern.len > 0 && pattern.data[0] == '^') return match_here((StringView){pattern.data + 1, pattern.len - 1}, text);
    
    // Try matching at every position in the text
    // Loop continues as long as there's text to try matching against.
    while (true) {
        if (match_here(pattern, text)) return true;
        if (text.len == 0) break; // No more text to try
        text.data++;
        text.len--;
    }
    return false;
}