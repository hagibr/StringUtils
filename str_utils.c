#include "str_utils.h"
#include <limits.h>
#include <float.h>

/* --- Basic StringView --- */

/**
 * Constructor functions are kept inline-friendly.
 * sv_from_parts allows creating views into sub-buffers.
 * sv_from_cstr handles NULL pointers gracefully to prevent crashes during initialization.
 */
StringView sv_from_parts(const char *data, size_t len) {
    return (StringView){data, len};
}
StringView sv_from_cstr(const char *s) {
    return (StringView){s, s ? strlen(s) : 0};
}

/**
 * Uses memcmp for binary-safe comparison. 
 * This is faster than character-by-character loops on most architectures.
 */
bool sv_equals(StringView a, StringView b) {
    return (a.len == b.len) && (memcmp(a.data, b.data, a.len) == 0);
}

/**
 * Avoids creating a temporary StringView for the C-string to reduce stack overhead.
 */
bool sv_equals_cstr(StringView a, const char *b) {
    if (!b) return false;
    size_t b_len = strlen(b);
    return (a.len == b_len) && (memcmp(a.data, b, a.len) == 0);
}

/**
 * Performs a "shrink-wrap" operation. 
 * It returns a new view without modifying the original data, making it O(1) in memory.
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
 * This follows a "consuming iterator" pattern. 
 * By passing a pointer to the input StringView, we advance the original reference 
 * forward, allowing users to call this in a simple 'while' loop.
 * It uses memchr for highly optimized delimiter searching.
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
 * FNV-1a (Fowler-Noll-Vo) 32-bit algorithm.
 * Chosen for its extremely low implementation footprint and excellent distribution 
 * properties for short-to-medium strings typical in hash table keys.
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
 * These wrappers use the internal engine and perform post-conversion 
 * bounds checking. This centralizes the parsing logic while ensuring type safety 
 * across different integer widths.
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
    if (sv_to_int64(sv, &v)) {
        if (v <= INT32_MAX && v >= INT32_MIN) {
            *res = (int32_t)v;
            return true;
        }
    }
    return false;
}

bool sv_to_int16(StringView sv, int16_t *res) {
    int64_t v;
    if (sv_to_int64(sv, &v)) {
        if (v <= INT16_MAX && v >= INT16_MIN) {
            *res = (int16_t)v;
            return true;
        }
    }
    return false;
}

bool sv_to_int8(StringView sv, int8_t *res) {
    int64_t v;
    if (sv_to_int64(sv, &v)) {
        if (v <= INT8_MAX && v >= INT8_MIN) {
            *res = (int8_t)v;
            return true;
        }
    }
    return false;
}

/* --- Strict Floating Point --- */

/**
 * Manual float parsing is implemented because standard library 
 * functions (strtod/atof) require a null-terminated string.
 * This implementation supports a single decimal point and optional sign.
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

bool sv_to_float32(StringView sv, float *res) {
    double v;
    if (sv_to_float64(sv, &v)) {
        if (v <= FLT_MAX && v >= -FLT_MAX) {
            *res = (float)v;
            return true;
        }
    }
    return false;
}

/* --- Hex and Protocols --- */

/**
 * Direct nibble conversion using bit shifts for maximum performance.
 */
bool sv_hex_to_uint8(StringView sv, uint8_t *res) {
    if (sv.len == 0 || sv.len > 2) return false;
    uint32_t val = 0;
    for (size_t i = 0; i < sv.len; i++) {
        char c = sv.data[i];
        val <<= 4;
        if (c >= '0' && c <= '9') {
            val |= (uint32_t)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            val |= (uint32_t)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            val |= (uint32_t)(c - 'A' + 10);
        } else {
            return false; // Invalid hex digit
        }
    }
    *res = (uint8_t)val; return true;
}

/**
 * Processes hex strings in pairs. 
 * It ensures the input length is even to prevent malformed byte interpretations.
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
 * Composes sv_split_next and sv_to_uint8. 
 * This demonstrates the power of StringView tokenization: no buffer copies are needed.
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
 * Flexible separator detection. 
 * It looks at the character at index 2 to determine the delimiter used 
 * (standardizes on ':' or '-' automatically).
 */
bool sv_parse_mac(StringView sv, uint8_t mac[6]) {
    if (sv.len != 17) return false;
    StringView in = sv;
    for (int i = 0; i < 6; i++) {
        StringView p = sv_split_next(&in, (i < 5) ? sv.data[2+(i*3)] : '\0');
        if (!sv_hex_to_uint8(p, &mac[i])) return false;
    }
    return true;
}

/* --- Shell Parsing --- */

/**
 * A non-destructive zero-copy parser.
 * Unlike strtok, it does not modify the input buffer. 
 * It handles quoted arguments by detecting the starting '"' and using 
 * sv_split_next to "jump" to the closing quote.
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
 * Safe string construction.
 * Every append operation checks remaining capacity and ensures the buffer 
 * is always null-terminated. This makes the internal data safe to use 
 * with legacy C functions if needed.
 */
StaticBuilder sb_init(char *buf, size_t size) {
    StaticBuilder sb = {buf, size, 0};
    if (size > 0) sb.data[0] = '\0';
    return sb;
}

void sb_reset(StaticBuilder *sb) {
    sb->len = 0;
    if (sb->capacity > 0) sb->data[0] = '\0';
}

/**
 * Uses memcpy for efficiency. 
 * Automatically manages the length and the trailing null byte.
 */
bool sb_append_sv(StaticBuilder *sb, StringView sv) {
    if (sb->len + sv.len + 1 > sb->capacity) return false;
    memcpy(sb->data + sb->len, sv.data, sv.len);
    sb->len += sv.len; sb->data[sb->len] = '\0';
    return true;
}

bool sb_append_cstr(StaticBuilder *sb, const char *s) {
    return sb_append_sv(sb, sv_from_cstr(s));
}

/**
 * Thin wrapper around vsnprintf.
 * It detects truncation by checking vsnprintf's return value against 
 * the remaining capacity, returning false if the message was too long.
 */
bool sb_append_fmt(StaticBuilder *sb, const char *fmt, ...) {
    if (sb->len + 1 >= sb->capacity) return false;
    va_list args; va_start(args, fmt);
    int res = vsnprintf(sb->data + sb->len, sb->capacity - sb->len, fmt, args);
    va_end(args);
    if (res < 0 || (size_t)res >= (sb->capacity - sb->len)) return false;
    sb->len += (size_t)res; return true;
}

StringView sb_to_view(const StaticBuilder *sb) {
    return (StringView){sb->data, sb->len};
}

/* --- Regex Utilities (Rob Pike's implementation) --- */

/**
 * @brief Internal helper function to match a regular expression against a text,
 * assuming the match starts at the current 'text' position.
 * 
 * Design: This is a recursive function that tries to match the current regex
 * character(s) against the current text character(s).
 * 
 * Early Exit / Failure Conditions:
 * - If regex is exhausted (re_l == 0), it's a match.
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
 * Design: It iteratively tries to match the rest of the regex (re + 2) against
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
 * Design: Handles '^' for anchoring the match to the beginning of the text.
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