/*
 * AFL++ fuzz target for url.c parser
 *
 * Build with AFL++:
 *   afl-gcc -o fuzz_afl fuzz_afl.c ../url.c -Wall -Wextra -ggdb
 *
 * Or with afl-clang-fast for better instrumentation:
 *   afl-clang-fast -o fuzz_afl fuzz_afl.c ../url.c -Wall -Wextra -ggdb
 *
 * Run with:
 *   afl-fuzz -i corpus/ -o findings/ -- ./fuzz_afl
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "../url.h"

// Maximum input size
#define MAX_INPUT_SIZE 8192

// Test buffer for outputs
static char output_buffer[16384];

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    // Read input from stdin (AFL++ feeds input this way)
    unsigned char input[MAX_INPUT_SIZE];
    ssize_t len = read(STDIN_FILENO, input, sizeof(input));

    if (len <= 0) {
        return 0;
    }

    // Ensure null termination for safety
    if (len < (ssize_t)sizeof(input)) {
        input[len] = '\0';
    }

    // Fuzz url_parse with different flag combinations
    URL url;
    int flags_to_test[] = {
        0,
        URL_FLAG_ALLOWREF,
        URL_FLAG_RFC3986,
        URL_FLAG_ALLOWREF | URL_FLAG_RFC3986
    };

    for (int i = 0; i < 4; i++) {
        int flags = flags_to_test[i];

        // Test with NULL cursor
        int ret = url_parse((char *)input, len, NULL, &url, flags);

        // Test with cursor
        int cursor = 0;
        url_parse((char *)input, len, &cursor, &url, flags);

        // If parse succeeded, test serialization
        if (ret == 0) {
            url_serialize(url, NULL, output_buffer, sizeof(output_buffer));

            // Test serialization with itself as base
            url_serialize(url, &url, output_buffer, sizeof(output_buffer));
        }
    }

    // Test whitespace removal
    int cleaned_len = url_remove_white_space((char *)input, len, output_buffer, sizeof(output_buffer));

    // If whitespace removal succeeded, try parsing the cleaned input
    if (cleaned_len > 0 && cleaned_len < (int)sizeof(output_buffer)) {
        url_parse(output_buffer, cleaned_len, NULL, &url, 0);
    }

    // Test percent decode on the full input
    URL_String str = { (char *)input, len };
    url_percent_decode(str, output_buffer, sizeof(output_buffer));

    // Test IPv4 parsing
    URL_IPv4 ipv4;
    url_parse_ipv4((char *)input, len, NULL, &ipv4);

    int cursor = 0;
    url_parse_ipv4((char *)input, len, &cursor, &ipv4);

    // Test IPv6 parsing
    URL_IPv6 ipv6;
    url_parse_ipv6((char *)input, len, NULL, &ipv6);

    cursor = 0;
    url_parse_ipv6((char *)input, len, &cursor, &ipv6);

    // Test parsing with base URL (if input is long enough)
    if (len > 10) {
        // Try to use first half as base, second half as reference
        int mid = len / 2;

        URL base_url;
        if (url_parse((char *)input, mid, NULL, &base_url, 0) == 0) {
            URL reference;
            if (url_parse((char *)input + mid, len - mid, NULL, &reference, URL_FLAG_ALLOWREF) == 0) {
                url_serialize(reference, &base_url, output_buffer, sizeof(output_buffer));
            }
        }
    }

    return 0;
}
