#ifndef STR_UTILS_H
#define STR_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

/* * StringView: Referência imutável para uma string (não garante null-termination).
 * Ideal para parsing sem copiar dados.
 */
typedef struct {
    const char *data;
    size_t len;
} StringView;

/* * StaticBuilder: Buffer fixo para montagem de strings com segurança.
 */
typedef struct {
    char *data;
    size_t capacity;
    size_t len;
} StaticBuilder;

/* --- Manipulação de StringView --- */
StringView sv_from_parts(const char *data, size_t len);
StringView sv_from_cstr(const char *s);
bool       sv_equals(StringView a, StringView b);
bool       sv_equals_cstr(StringView a, const char *b);
StringView sv_trim(StringView sv);
StringView sv_split_next(StringView *input, char delim);
uint32_t   sv_hash(StringView sv);

/* --- Conversão Numérica Estrita (Suporta 0x, 0b e _) --- */
bool sv_to_int64(StringView sv, int64_t *res);
bool sv_to_int32(StringView sv, int32_t *res);
bool sv_to_int16(StringView sv, int16_t *res);
bool sv_to_int8(StringView sv, int8_t *res);

bool sv_to_uint64(StringView sv, uint64_t *res);
bool sv_to_uint32(StringView sv, uint32_t *res);
bool sv_to_uint16(StringView sv, uint16_t *res);
bool sv_to_uint8(StringView sv, uint8_t *res);

bool sv_to_float64(StringView sv, double *res);
bool sv_to_float32(StringView sv, float *res);

/* --- Utilitários de Hex e Protocolos --- */
bool sv_hex_to_uint8(StringView sv, uint8_t *res);
bool sv_hex_to_uint8_array(StringView sv, uint8_t *array, size_t *array_size);
bool sv_parse_ipv4(StringView sv, uint8_t ip[4]);
bool sv_parse_mac(StringView sv, uint8_t mac[6]);

/* --- Parsing de Shell --- */
int  shell_parse_line(char *line, StringView argv[], int max_args);

/* --- StaticBuilder --- */
StaticBuilder sb_init(char *external_buffer, size_t size);
void          sb_reset(StaticBuilder *sb);
bool          sb_append_sv(StaticBuilder *sb, StringView sv);
bool          sb_append_cstr(StaticBuilder *sb, const char *s);
bool          sb_append_fmt(StaticBuilder *sb, const char *fmt, ...);
StringView    sb_to_view(const StaticBuilder *sb);

/* Macros para printf amigável */
#define PRIsv "%.*s"
#define EXsv(sv) (int)(sv).len, (sv).data

#endif // STR_UTILS_H