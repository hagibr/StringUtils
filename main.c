#include <stdio.h>
#include <stdlib.h>
#include "str_utils.h"

#define MAX_CMD_LEN 1024

int main(void) {
    char line_buf[MAX_CMD_LEN];
    
    printf("StringUtils Interactive CLI (type 'exit'/'quit' to quit)\n");
    printf("Type 'help'/'?' to see the available commands.\n\n");

    while (1) {
        printf("> ");
        fflush(stdout); // Ensure the prompt is displayed immediately

        if (!fgets(line_buf, sizeof(line_buf), stdin)) {
            printf("\nExiting.\n");
            break;
        }

        // Parsing the line and splitting into several StringViews (max 10)
        StringView args[10];
        int count = shell_parse_line(line_buf, args, 10);

        // Empty line
        if (count == 0) continue;

        // Now we execute thecommands. Optimizing by using the hash functions.
        switch (sv_hash(args[0])) {
            case 0xcded1a85: // "exit"
            case 0x47878736: // "quit"
                printf("Exiting.\n");
                return 0;

            case 0x3871a3fa: // "help"
            case 0x3a0cb08e: // "?""
                printf("Available commands:\n");
                printf("  int64, int32, int16, int8 <val>     : Parse signed integers (supports 0x, 0b, _)\n");
                printf("  uint64, uint32, uint16, uint8 <val> : Parse unsigned integers (supports 0x, 0b, _)\n");
                printf("  float64, float32 <val>              : Parse floating-point numbers\n");
                printf("  hex <string> [limit]                : Validate hex byte or parse byte array\n");
                printf("  ipv4 <address>                      : Validate IPv4 address string\n");
                printf("  mac <address>                       : Validate MAC address string\n");
                printf("  hash <string>                       : Calculate FNV-1a 32-bit hash\n");
                printf("  split <string> <char>               : Split string by a single delimiter\n");
                printf("  regex|match <pattern> <text>        : Match text against a simple regex pattern\n");
                printf("  build <args...>                     : Test StaticBuilder by joining arguments\n");
                printf("  help, ?                             : Show this list\n");
                printf("  exit, quit                          : Quit the program\n");
                break;

            case 0x03d22364: // "int64"
                if (count < 2) { printf("Usage: int64 <value>\n"); break; }
                int64_t v64;
                if (sv_to_int64(args[1], &v64)) printf("Result: Valid int64 -> %lld\n", (long long)v64);
                else printf("Result: Invalid int64\n");
                break;

            case 0xfbdee2bf: // "int32"
                if (count < 2) { printf("Usage: int32 <value>\n"); break; }
                int32_t v32;
                if (sv_to_int32(args[1], &v32)) printf("Result: Valid int32 -> %d\n", v32);
                else printf("Result: Invalid int32\n");
                break;

            case 0x07e372d1: // "int16"
                if (count < 2) { printf("Usage: int16 <value>\n"); break; }
                int16_t v16;
                if (sv_to_int16(args[1], &v16)) printf("Result: Valid int16 -> %d\n", v16);
                else printf("Result: Invalid int16\n");
                break;

            case 0x6491fa92: // "int8"
                if (count < 2) { printf("Usage: int8 <value>\n"); break; }
                int8_t v8;
                if (sv_to_int8(args[1], &v8)) printf("Result: Valid int8 -> %d\n", (int)v8);
                else printf("Result: Invalid int8\n");
                break;

            case 0xaea00813: // "uint64"
                if (count < 2) { printf("Usage: uint64 <value>\n"); break; }
                uint64_t u64;
                if (sv_to_uint64(args[1], &u64)) printf("Result: Valid uint64 -> %llu\n", (unsigned long long)u64);
                else printf("Result: Invalid uint64\n");
                break;

            case 0x32940bec: // "uint32"
                if (count < 2) { printf("Usage: uint32 <value>\n"); break; }
                uint32_t u32;
                if (sv_to_uint32(args[1], &u32)) printf("Result: Valid uint32 -> %u\n", u32);
                else printf("Result: Invalid uint32\n");
                break;

            case 0xae8ebef2: // "uint16"
                if (count < 2) { printf("Usage: uint16 <value>\n"); break; }
                uint16_t u16;
                if (sv_to_uint16(args[1], &u16)) printf("Result: Valid uint16 -> %u\n", u16);
                else printf("Result: Invalid uint16\n");
                break;

            case 0x199df2db: // "uint8"
                if (count < 2) { printf("Usage: uint8 <value>\n"); break; }
                uint8_t u8;
                if (sv_to_uint8(args[1], &u8)) printf("Result: Valid uint8 -> %u\n", u8);
                else printf("Result: Invalid uint8\n");
                break;

            case 0x7c980e47: // "float64"
                if (count < 2) { printf("Usage: float64 <value>\n"); break; }
                double f64;
                if (sv_to_float64(args[1], &f64)) printf("Result: Valid float64 -> %lf\n", f64);
                else printf("Result: Invalid float64\n");
                break;

            case 0xe89f7410: // "float32"
                if (count < 2) { printf("Usage: float32 <value>\n"); break; }
                float f32;
                if (sv_to_float32(args[1], &f32)) printf("Result: Valid float32 -> %f\n", f32);
                else printf("Result: Invalid float32\n");
                break;

            case 0xfeb49d4a: // "hex"
                if (count == 2) {
                    uint8_t val;
                    if (sv_hex_to_uint8(args[1], &val)) printf("Result: Valid hex byte -> %u (0x%02X)\n", val, val);
                    else printf("Result: Invalid hex byte\n");
                } else if (count == 3) {
                    uint32_t limit;
                    if (!sv_to_uint32(args[2], &limit)) { printf("Error: Invalid limit\n"); break; }
                    if (limit > 256) limit = 256;
                    uint8_t buf[256]; size_t sz = (size_t)limit;
                    if (sv_hex_to_uint8_array(args[1], buf, &sz)) {
                        printf("Result: Valid hex array (%zu bytes): ", sz);
                        for (size_t i = 0; i < sz; i++) printf("%02X ", buf[i]);
                        printf("\n");
                    } else printf("Result: Invalid hex string or limit exceeded\n");
                } else printf("Usage: hex <string> [limit]\n");
                break;

            case 0x285cc21e: // "ipv4"
                if (count < 2) { printf("Usage: ipv4 <addr>\n"); break; }
                uint8_t ip[4];
                if (sv_parse_ipv4(args[1], ip)) printf("Result: Valid IPv4 -> %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
                else printf("Result: Invalid IPv4\n");
                break;

            case 0xeca30428: // "mac"
                if (count < 2) { printf("Usage: mac <addr>\n"); break; }
                uint8_t mac[6];
                if (sv_parse_mac(args[1], mac)) {
                    printf("Result: Valid MAC -> %02X:%02X:%02X:%02X:%02X:%02X\n",
                           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                } else printf("Result: Invalid MAC\n");
                break;

            case 0xcec577d1: // "hash"
                if (count < 2) { printf("Usage: hash <string>\n"); break; }
                printf("Result: FNV-1a Hash -> 0x%08X\n", sv_hash(args[1]));
                break;

            case 0x87b82de3: // "split"
                if (count < 3 || args[2].len != 1) { printf("Usage: split <str> <char>\n"); break; }
                char delim = args[2].data[0];
                StringView input = args[1];
                printf("Splitting \"" PRIsv "\" by '%c':\n", EXsv(input), delim);
                while (input.len > 0) {
                    StringView part = sv_split_next(&input, delim);
                    printf("  - \"" PRIsv "\"\n", EXsv(part));
                }
                break;

            case 0xeaff1078: // "regex"
            case 0x7E202F96: // "match"
                if (count < 3) { printf("Usage: regex|match <pattern> <text>\n"); break; }
                bool matched = sv_match(args[1], args[2]);
                printf("Result: %s\n", matched ? "true" : "false");
                break;

            case 0xc39bf2a3: // "build"
                {
                    char builder_buf[256];
                    StaticBuilder sb = sb_init(builder_buf, sizeof(builder_buf));
                    sb_append_cstr(&sb, "Result: [");
                    for (int i = 1; i < count; i++) {
                        sb_append_sv(&sb, args[i]);
                        if (i < count - 1) sb_append_cstr(&sb, ", ");
                    }
                    sb_append_cstr(&sb, "]");
                    printf("" PRIsv "\n", EXsv(sb_to_view(&sb)));
                }
                break;

            default:
                printf("Unknown command: " PRIsv "\n", EXsv(args[0]));
                break;
        }
    }

    return 0;
}