#include "str_utils.h"
#include <limits.h>
#include <float.h>

/* --- StringView Básico --- */

StringView sv_from_parts(const char *data, size_t len) { return (StringView){data, len}; }
StringView sv_from_cstr(const char *s) { return (StringView){s, s ? strlen(s) : 0}; }

bool sv_equals(StringView a, StringView b) {
    return (a.len == b.len) && (memcmp(a.data, b.data, a.len) == 0);
}

bool sv_equals_cstr(StringView a, const char *b) {
    if (!b) return false;
    size_t b_len = strlen(b);
    return (a.len == b_len) && (memcmp(a.data, b, a.len) == 0);
}

StringView sv_trim(StringView sv) {
    while (sv.len > 0 && isspace((unsigned char)*sv.data)) { sv.data++; sv.len--; }
    while (sv.len > 0 && isspace((unsigned char)sv.data[sv.len - 1])) { sv.len--; }
    return sv;
}

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

uint32_t sv_hash(StringView sv) {
    uint32_t hash = 0x811c9dc5;
    for (size_t i = 0; i < sv.len; i++) {
        hash ^= (uint8_t)sv.data[i];
        hash *= 0x01000193;
    }
    return hash;
}

/* --- Motor de Conversão Inteira --- */

static bool sv_internal_to_u64(StringView sv, uint64_t *result, bool *neg) {
    if (sv.len == 0) return false;
    size_t i = 0; uint64_t val = 0; int base = 10; *neg = false;

    if (sv.data[i] == '-') { *neg = true; i++; }
    else if (sv.data[i] == '+') i++;

    if (i + 2 <= sv.len && sv.data[i] == '0') {
        char p = sv.data[i+1];
        if (p == 'x' || p == 'X') { base = 16; i += 2; }
        else if (p == 'b' || p == 'B') { base = 2; i += 2; }
    }
    if (i == sv.len) return false;

    bool has_digits = false;
    for (; i < sv.len; i++) {
        char c = sv.data[i];
        if (c == '_') continue;
        uint8_t digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (base == 16 && (c|32) >= 'a' && (c|32) <= 'f') digit = (c|32) - 'a' + 10;
        else return false; // Falha estrita em qualquer char inválido

        if (digit >= base) return false; // Ex: '2' em binário
        if (val > (UINT64_MAX / base)) return false; // Overflow
        val = (val * base) + digit;
        has_digits = true;
    }
    if (!has_digits) return false;
    *result = val; return true;
}

// Wrappers com Range Check
bool sv_to_int64(StringView sv, int64_t *res) {
    uint64_t v; bool n; if (!sv_internal_to_u64(sv, &v, &n)) return false;
    if (!n && v > (uint64_t)INT64_MAX) return false;
    if (n && v > (uint64_t)INT64_MAX + 1) return false;
    *res = n ? -(int64_t)v : (int64_t)v; return true;
}
bool sv_to_int32(StringView sv, int32_t *res) { int64_t v; return (sv_to_int64(sv, &v) && v <= INT32_MAX && v >= INT32_MIN) ? (*res = (int32_t)v, true) : false; }
bool sv_to_uint8(StringView sv, uint8_t *res)   { uint64_t v; bool n; return (sv_internal_to_u64(sv, &v, &n) && !n && v <= 255) ? (*res = (uint8_t)v, true) : false; }
/* ... (outros tamanhos seguem a mesma lógica de range check) ... */

/* --- Ponto Flutuante Estrito --- */

bool sv_to_float64(StringView sv, double *res) {
    if (sv.len == 0) return false;
    size_t i = 0; double val = 0.0, sign = 1.0;
    if (sv.data[i] == '-') { sign = -1.0; i++; }
    else if (sv.data[i] == '+') i++;

    bool has_digits = false, dot_seen = false; double weight = 0.1;
    for (; i < sv.len; i++) {
        char c = sv.data[i];
        if (c == '_') continue;
        if (c == '.') { if (dot_seen) return false; dot_seen = true; continue; }
        if (!isdigit((unsigned char)c)) return false;
        if (!dot_seen) val = val * 10.0 + (c - '0');
        else { val += (c - '0') * weight; weight /= 10.0; }
        has_digits = true;
    }
    if (!has_digits) return false;
    *res = val * sign; return true;
}

bool sv_to_float32(StringView sv, float *res) { 
    double v; return (sv_to_float64(sv, &v) && v <= FLT_MAX && v >= -FLT_MAX) ? (*res = (float)v, true) : false; 
}

/* --- Hex e Protocolos --- */

bool sv_hex_to_uint8(StringView sv, uint8_t *res) {
    if (sv.len == 0 || sv.len > 2) return false;
    uint32_t val = 0;
    for (size_t i = 0; i < sv.len; i++) {
        char c = sv.data[i]; val <<= 4;
        if (isdigit(c)) val |= (c - '0');
        else if ((c|32) >= 'a' && (c|32) <= 'f') val |= ((c|32) - 'a' + 10);
        else return false;
    }
    *res = (uint8_t)val; return true;
}

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

bool sv_parse_ipv4(StringView sv, uint8_t ip[4]) {
    StringView in = sv;
    for (int i = 0; i < 4; i++) {
        StringView p = sv_split_next(&in, '.');
        if (!sv_to_uint8(p, &ip[i])) return false;
    }
    return in.len == 0;
}

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

int shell_parse_line(char *line, StringView argv[], int max_args) {
    StringView input = sv_trim(sv_from_cstr(line));
    int argc = 0;
    while (argc < max_args && input.len > 0) {
        if (input.data[0] == '"') {
            input.data++; input.len--; // Pula "
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

StaticBuilder sb_init(char *buf, size_t size) {
    StaticBuilder sb = {buf, size, 0};
    if (size > 0) sb.data[0] = '\0';
    return sb;
}

void sb_reset(StaticBuilder *sb) { sb->len = 0; if (sb->capacity > 0) sb->data[0] = '\0'; }

bool sb_append_sv(StaticBuilder *sb, StringView sv) {
    if (sb->len + sv.len + 1 > sb->capacity) return false;
    memcpy(sb->data + sb->len, sv.data, sv.len);
    sb->len += sv.len; sb->data[sb->len] = '\0';
    return true;
}

bool sb_append_cstr(StaticBuilder *sb, const char *s) {
    return sb_append_sv(sb, sv_from_cstr(s));
}

bool sb_append_fmt(StaticBuilder *sb, const char *fmt, ...) {
    if (sb->len + 1 >= sb->capacity) return false;
    va_list args; va_start(args, fmt);
    int res = vsnprintf(sb->data + sb->len, sb->capacity - sb->len, fmt, args);
    va_end(args);
    if (res < 0 || (size_t)res >= (sb->capacity - sb->len)) return false;
    sb->len += (size_t)res; return true;
}

StringView sb_to_view(const StaticBuilder *sb) { return (StringView){sb->data, sb->len}; }