#include "str_utils.h"
#include <limits.h>
#include <float.h>

StringView sv_from_parts(const char *data, size_t len) {
    // Creates a view over an existing buffer slice — no copy, no null-terminator needed
    return (StringView){data, len};
}

StringView sv_from_cstr(const char *s) {
    // Accepts NULL safely, returning an empty view instead of crashing
    return (StringView){s, s ? strlen(s) : 0};
}

bool sv_equals(StringView a, StringView b) {
    // Short-circuit on length mismatch before touching the data
    return (a.len == b.len) && (memcmp(a.data, b.data, a.len) == 0);
}

bool sv_equals_cstr(StringView a, const char *b) {
    if (!b) return false; // Treat NULL as not equal
    size_t b_len = strlen(b);
    return (a.len == b_len) && (memcmp(a.data, b, a.len) == 0);
}

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
    return sv; // Only the pointer and length change — the buffer is untouched
}

StringView sv_split_next(StringView *input, char delim) {
    if (input->len == 0) return (StringView){NULL, 0};
    const char *start = input->data;
    const char *end = memchr(start, delim, input->len); // Optimised delimiter search
    if (!end) {
        // Delimiter not found: return the whole remaining view and mark input as consumed
        StringView token = *input;
        input->data += input->len; input->len = 0;
        return token;
    }
    size_t token_len = end - start;
    StringView token = {start, token_len};
    input->data = end + 1; input->len -= (token_len + 1); // Advance past the delimiter
    return token;
}

uint32_t sv_hash(StringView sv) {
    // FNV-1a 32-bit: XOR then multiply for each byte
    uint32_t hash = 0x811c9dc5;
    for (size_t i = 0; i < sv.len; i++) {
        hash ^= (uint8_t)sv.data[i];
        hash *= 0x01000193;
    }
    return hash;
}

static bool sv_internal_to_u64(StringView sv, uint64_t *result, bool *neg) {
    if (sv.len == 0) return false; // Exit 1: Empty input
    size_t i = 0; uint64_t val = 0; int base = 10; *neg = false;

    // Handle optional leading sign
    if (sv.data[i] == '-') { *neg = true; i++; }
    else if (sv.data[i] == '+') i++;

    // Detect base prefix: 0x → hex, 0b → binary, default → decimal
    if (i + 2 <= sv.len && sv.data[i] == '0') {
        char p = sv.data[i+1];
        if (p == 'x' || p == 'X') { base = 16; i += 2; }
        else if (p == 'b' || p == 'B') { base = 2; i += 2; }
    }
    if (i == sv.len) return false; // Exit 2: Input is just a sign or prefix with no digits

    bool has_digits = false;
    for (; i < sv.len; i++) {
        char c = sv.data[i];
        if (c == '_') continue; // Underscores are visual separators, skip them
        uint8_t digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (base == 16 && (c|32) >= 'a' && (c|32) <= 'f') digit = (c|32) - 'a' + 10;
        else return false; // Exit 3: Character is not valid for the current base

        if (digit >= (uint8_t)base) return false; // Exit 4: Digit value exceeds base (e.g. '2' in binary)
        if (val > (UINT64_MAX / (uint64_t)base)) return false; // Exit 5: Next multiply would overflow
        val = (val * base) + digit;
        has_digits = true;
    }
    if (!has_digits) return false; // Exit 6: Only separators were found (e.g. "0x_")
    *result = val; return true;
}

bool sv_to_uint64(StringView sv, uint64_t *res) {
    uint64_t v;
    bool n;
    if (sv_internal_to_u64(sv, &v, &n) && !n) { // Reject negative values
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
    // INT64_MIN is -(2^63), which is one more than INT64_MAX in absolute value
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

bool sv_to_float64(StringView sv, double *res) {
    // strtod/atof require null-termination, so we parse manually
    if (sv.len == 0) {
        return false;
    }

    size_t i = 0;
    double val = 0.0;
    double sign = 1.0;

    // Handle optional leading sign
    if (sv.data[i] == '-') {
        sign = -1.0;
        i++;
    } else if (sv.data[i] == '+') {
        i++;
    }

    bool has_digits = false;
    bool dot_seen = false;
    double weight = 0.1;

    for (; i < sv.len; i++) {
        char c = sv.data[i];

        if (c == '_') continue; // Underscore separators are skipped (e.g. 1_000.50)

        if (c == '.') {
            if (dot_seen) return false; // Only one decimal point is allowed
            dot_seen = true;
            continue;
        }

        if (!isdigit((unsigned char)c)) {
            return false;
        }

        if (!dot_seen) {
            val = val * 10.0 + (c - '0'); // Integer part: shift left and add digit
        } else {
            val += (c - '0') * weight;    // Fractional part: weight decreases with each position
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
    // Parse as double first, then check the value fits within float range (±FLT_MAX)
    double v;
    if (sv_to_float64(sv, &v) && v <= FLT_MAX && v >= -FLT_MAX) {
        *res = (float)v;
        return true;
    }
    return false;
}

bool sv_hex_to_uint8(StringView sv, uint8_t *res) {
    if (sv.len == 0 || sv.len > 2) return false; // Only 1 or 2 hex chars map to a single byte
    uint32_t val = 0;
    for (size_t i = 0; i < sv.len; i++) {
        char c = sv.data[i];
        val <<= 4; // Shift existing nibble up to make room for the next
        if (c >= '0' && c <= '9') {
            val |= (uint32_t)(c - '0');
        } else if ((c|32) >= 'a' && (c|32) <= 'f') {
            val |= (uint32_t)((c|32) - 'a' + 10); // (c|32) lowercases without branching
        } else {
            return false; // Invalid hex digit
        }
    }
    *res = (uint8_t)val; return true;
}

bool sv_hex_to_uint8_array(StringView sv, uint8_t *array, size_t *array_size) {
    if (!array || !array_size || sv.len % 2 != 0) return false; // Odd length would leave a dangling nibble
    size_t needed = sv.len / 2;
    if (needed > *array_size) return false; // Output buffer too small
    for (size_t i = 0; i < needed; i++) {
        StringView b = {sv.data + (i * 2), 2}; // Slice each pair of hex chars
        if (!sv_hex_to_uint8(b, &array[i])) return false;
    }
    *array_size = needed; return true; // Update caller with the actual number of bytes written
}

bool sv_parse_ipv4(StringView sv, uint8_t ip[4]) {
    StringView in = sv;
    for (int i = 0; i < 4; i++) {
        StringView p = sv_split_next(&in, '.'); // Extract each octet between dots
        if (!sv_to_uint8(p, &ip[i])) return false;
    }
    return in.len == 0; // Fail if there are leftover characters after the 4th octet
}

bool sv_parse_mac(StringView sv, uint8_t mac[6]) {
    if (sv.len != 17) return false; // "XX:XX:XX:XX:XX:XX" is always exactly 17 chars
    char delim = sv.data[2]; // Detect delimiter from the position after the first byte
    if (delim != ':' && delim != '-') return false;

    StringView in = sv;
    for (int i = 0; i < 6; i++) {
        if (i < 5 && sv.data[2 + (i * 3)] != delim) return false; // Enforce consistent delimiter
        StringView p = sv_split_next(&in, (i < 5) ? delim : '\0');
        if (!sv_hex_to_uint8(p, &mac[i])) return false;
    }
    return in.len == 0;
}

int shell_parse_line(char *line, StringView argv[], int max_args) {
    StringView input = sv_trim(sv_from_cstr(line)); // Strip leading/trailing whitespace first
    int argc = 0;
    while (argc < max_args && input.len > 0) {
        if (input.data[0] == '"') {
            input.data++; input.len--;              // Skip the opening quote
            StringView arg = sv_split_next(&input, '"'); // Read until the closing quote
            argv[argc++] = arg;
            // Note: escaped quotes are not supported — that would require modifying the
            // buffer or allocating memory, both undesirable in an embedded context
        } else {
            const char *start = input.data; size_t l = 0;
            while (l < input.len && !isspace((unsigned char)start[l])) l++; // Scan to next whitespace
            argv[argc++] = (StringView){start, l};
            input.data += l; input.len -= l;
        }
        input = sv_trim(input); // Consume whitespace between tokens
    }
    return argc;
}

StaticBuilder sb_init(char *buf, size_t size) {
    StaticBuilder sb = {buf, size, 0};
    if (size > 0) sb.data[0] = '\0'; // Ensure the buffer is a valid empty C-string immediately
    return sb;
}

void sb_reset(StaticBuilder *sb) {
    sb->len = 0;
    if (sb->capacity > 0) sb->data[0] = '\0'; // Reuse the same buffer from the start
}

bool sb_append_sv(StaticBuilder *sb, StringView sv) {
    if (sb->len + sv.len + 1 > sb->capacity) return false; // +1 for the null terminator
    memcpy(sb->data + sb->len, sv.data, sv.len);
    sb->len += sv.len; sb->data[sb->len] = '\0'; // Always keep the buffer null-terminated
    return true;
}

bool sb_append_cstr(StaticBuilder *sb, const char *s) {
    return sb_append_sv(sb, sv_from_cstr(s)); // Thin wrapper — delegates length calculation to sv_from_cstr
}

bool sb_append_fmt(StaticBuilder *sb, const char *fmt, ...) {
    if (sb->len + 1 >= sb->capacity) return false;
    va_list args; va_start(args, fmt);
    int res = vsnprintf(sb->data + sb->len, sb->capacity - sb->len, fmt, args);
    va_end(args);
    // vsnprintf returns the number of chars it would have written; if >= remaining space, it truncated
    if (res < 0 || (size_t)res >= (sb->capacity - sb->len)) return false;
    sb->len += (size_t)res; return true;
}

StringView sb_to_view(const StaticBuilder *sb) {
    return (StringView){sb->data, sb->len}; // The view shares the builder's buffer — no copy
}

static bool match_here(StringView pattern, StringView text);

static bool match_star(char c, StringView pattern, StringView text) {
    do {
        // Try to match the rest of the pattern against the current text position (zero occurrences of c)
        if (match_here(pattern, text)) return true;

        // Stop if text is exhausted or the current char doesn't match c ('.' matches anything)
        if (text.len == 0 || (c != '.' && *text.data != c)) break;

        // Consume one occurrence of c and retry
        text.data++;
        text.len--;
    } while (true);
    return false;
}

static bool match_here(StringView pattern, StringView text) {
    if (pattern.len == 0) return true; // Pattern exhausted — everything matched

    // '*' quantifier: delegate to match_star with the char before '*' and the rest of the pattern
    if (pattern.len >= 2 && pattern.data[1] == '*') {
        return match_star(pattern.data[0], (StringView){pattern.data + 2, pattern.len - 2}, text);
    }

    // '$' at end of pattern: match only if text is also exhausted
    if (pattern.len == 1 && pattern.data[0] == '$') {
        return text.len == 0;
    }

    // '.' matches any single character; otherwise require an exact character match
    if (text.len > 0 && (pattern.data[0] == '.' || pattern.data[0] == *text.data)) {
        return match_here((StringView){pattern.data + 1, pattern.len - 1}, (StringView){text.data + 1, text.len - 1});
    }

    return false;
}

bool sv_match(StringView pattern, StringView text) {
    // '^' anchors the match to the start — skip it and match only at position 0
    if (pattern.len > 0 && pattern.data[0] == '^') return match_here((StringView){pattern.data + 1, pattern.len - 1}, text);

    // Without '^', try matching at every position in the text
    while (true) {
        if (match_here(pattern, text)) return true;
        if (text.len == 0) break; // No more positions to try
        text.data++;
        text.len--;
    }
    return false;
}

ByteStream bs_init(char *buf, size_t size) {
    // One slot is sacrificed as a sentinel so head==tail unambiguously means empty
    return (ByteStream){ buf, size - 1, 0, 0 };
}

void bs_reset(ByteStream *bs) {
    bs->head = bs->tail = 0; // Both cursors back to zero — buffer contents are abandoned
}

size_t bs_readable(const ByteStream *bs) {
    // Derived from head and tail only — no shared 'len' field, safe for single-producer/single-consumer
    return (bs->head - bs->tail + bs->capacity + 1) % (bs->capacity + 1);
}

size_t bs_writable(const ByteStream *bs) {
    return bs->capacity - bs_readable(bs); // Usable capacity is always one less than the buffer size
}

size_t bs_write(ByteStream *bs, const char *data, size_t len) {
    size_t n = len < bs_writable(bs) ? len : bs_writable(bs); // Clamp to available space
    for (size_t i = 0; i < n; i++) {
        bs->buf[bs->head] = data[i];
        bs->head = (bs->head + 1) % (bs->capacity + 1); // Wrap head around the physical buffer
    }
    return n;
}

size_t bs_peek(const ByteStream *bs, char *dst, size_t len) {
    size_t n = len < bs_readable(bs) ? len : bs_readable(bs); // Clamp to available data
    for (size_t i = 0; i < n; i++) {
        dst[i] = bs->buf[(bs->tail + i) % (bs->capacity + 1)]; // Wrap-aware read, tail is not moved
    }
    return n;
}

size_t bs_read(ByteStream *bs, char *dst, size_t len) {
    size_t n = bs_peek(bs, dst, len);                          // Reuse peek to avoid duplicating wrap logic
    bs->tail = (bs->tail + n) % (bs->capacity + 1);           // Advance tail only after data is copied
    return n;
}
