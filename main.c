#include <stdio.h>
#include <stdlib.h>
#include "str_utils.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    char *line = NULL;
    size_t n = 0;

    printf("StringUtils Interactive CLI (type 'exit' to quit)\n");
    printf("Type 'help' to see the available commands.\n\n");

    while (getline(&line, &n, stdin) != -1) {
        StringView args[10];
        int count = shell_parse_line(line, args, 10);

        if (count == 0) continue;

        if (sv_equals_cstr(args[0], "exit")) {
            break;
        }

        /* 0. Command: help */
        if (sv_equals_cstr(args[0], "help")) {
            printf("Available commands:\n");
            printf("  int64, int32, int16, int8 <val>   : Parse signed integers (supports 0x, 0b, _)\n");
            printf("  uint64, uint32, uint16, uint8 <val>: Parse unsigned integers (supports 0x, 0b, _)\n");
            printf("  hex <string> [limit]               : Validate hex byte or parse byte array\n");
            printf("  ipv4 <address>                     : Validate IPv4 address string\n");
            printf("  hash <string>                      : Calculate FNV-1a 32-bit hash\n");
            printf("  split <string> <char>              : Split string by a single delimiter\n");
            printf("  help                               : Show this list\n");
            printf("  exit                               : Quit the program\n");
            continue;
        }

        /* Signed Integer Commands */
        if (sv_equals_cstr(args[0], "int64")) {
            if (count < 2) { printf("Usage: int64 <value>\n"); continue; }
            int64_t val;
            if (sv_to_int64(args[1], &val)) printf("Result: Valid int64 -> %lld\n", (long long)val);
            else printf("Result: Invalid int64\n");
        }
        else if (sv_equals_cstr(args[0], "int32")) {
            if (count < 2) { printf("Usage: int32 <value>\n"); continue; }
            int32_t val;
            if (sv_to_int32(args[1], &val)) printf("Result: Valid int32 -> %d\n", val);
            else printf("Result: Invalid int32\n");
        }
        else if (sv_equals_cstr(args[0], "int16")) {
            if (count < 2) { printf("Usage: int16 <value>\n"); continue; }
            int16_t val;
            if (sv_to_int16(args[1], &val)) printf("Result: Valid int16 -> %d\n", val);
            else printf("Result: Invalid int16\n");
        }
        else if (sv_equals_cstr(args[0], "int8")) {
            if (count < 2) { printf("Usage: int8 <value>\n"); continue; }
            int8_t val;
            if (sv_to_int8(args[1], &val)) printf("Result: Valid int8 -> %d\n", (int)val);
            else printf("Result: Invalid int8\n");
        }

        /* Unsigned Integer Commands */
        else if (sv_equals_cstr(args[0], "uint64")) {
            if (count < 2) { printf("Usage: uint64 <value>\n"); continue; }
            uint64_t val;
            if (sv_to_uint64(args[1], &val)) printf("Result: Valid uint64 -> %llu\n", (unsigned long long)val);
            else printf("Result: Invalid uint64\n");
        }
        else if (sv_equals_cstr(args[0], "uint32")) {
            if (count < 2) { printf("Usage: uint32 <value>\n"); continue; }
            uint32_t val;
            if (sv_to_uint32(args[1], &val)) printf("Result: Valid uint32 -> %u\n", val);
            else printf("Result: Invalid uint32\n");
        }
        else if (sv_equals_cstr(args[0], "uint16")) {
            if (count < 2) { printf("Usage: uint16 <value>\n"); continue; }
            uint16_t val;
            if (sv_to_uint16(args[1], &val)) printf("Result: Valid uint16 -> %u\n", val);
            else printf("Result: Invalid uint16\n");
        }
        /* 1. Command: uint8 <value> */
        else if (sv_equals_cstr(args[0], "uint8")) {
            if (count < 2) {
                printf("Usage: uint8 <value>\n");
                continue;
            }
            uint8_t val;
            if (sv_to_uint8(args[1], &val)) {
                printf("Result: Valid uint8 -> %u\n", val);
            } else {
                printf("Result: Invalid uint8\n");
            }
        }
        /* 2. Command: hex <string> [max_limit] */
        else if (sv_equals_cstr(args[0], "hex")) {
            if (count == 2) {
                // Single byte hex validation
                uint8_t val;
                if (sv_hex_to_uint8(args[1], &val)) {
                    printf("Result: Valid hex byte -> %u (0x%02X)\n", val, val);
                } else {
                    printf("Result: Invalid hex byte\n");
                }
            } else if (count == 3) {
                // Hex array with capacity limit
                uint32_t limit;
                if (!sv_to_uint32(args[2], &limit)) {
                    printf("Error: Invalid limit value\n");
                    continue;
                }
                if (limit > 256) limit = 256;

                uint8_t buffer[256];
                size_t actual_size = (size_t)limit;
                
                if (sv_hex_to_uint8_array(args[1], buffer, &actual_size)) {
                    printf("Result: Valid hex array (%zu bytes): ", actual_size);
                    for (size_t i = 0; i < actual_size; i++) {
                        printf("%02X ", buffer[i]);
                    }
                    printf("\n");
                } else {
                    printf("Result: Invalid hex string or limit exceeded\n");
                }
            } else {
                printf("Usage: hex <string> OR hex <string> <limit>\n");
            }
        }
        /* 3. Command: ipv4 <address> */
        else if (sv_equals_cstr(args[0], "ipv4")) {
            if (count < 2) { printf("Usage: ipv4 <addr>\n"); continue; }
            uint8_t ip[4];
            if (sv_parse_ipv4(args[1], ip)) {
                printf("Result: Valid IPv4 -> %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
            } else {
                printf("Result: Invalid IPv4\n");
            }
        }
        /* 4. Command: hash <string> */
        else if (sv_equals_cstr(args[0], "hash")) {
            if (count < 2) { printf("Usage: hash <string>\n"); continue; }
            printf("Result: FNV-1a Hash -> 0x%08X\n", sv_hash(args[1]));
        }
        /* 5. Command: split <string> <delimiter> */
        else if (sv_equals_cstr(args[0], "split")) {
            if (count < 3) { printf("Usage: split <string> <char>\n"); continue; }
            if (args[2].len != 1) {
                printf("Error: Delimiter must be exactly one character.\n");
                continue;
            }
            char delim = args[2].data[0];
            StringView input = args[1];
            printf("Splitting \"" PRIsv "\" by '%c':\n", EXsv(input), delim);
            while (input.len > 0) {
                StringView part = sv_split_next(&input, delim);
                printf("  - \"" PRIsv "\"\n", EXsv(part));
            }
        } else {
            printf("Unknown command: " PRIsv "\n", EXsv(args[0]));
        }
    }

    free(line);
    return 0;
}