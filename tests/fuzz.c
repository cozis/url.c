#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#include "../url.h"

// PRNG state
static uint64_t rng_state = 0;

// Simple xorshift64 RNG
static uint64_t rand64(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

// Generate random byte
static uint8_t rand_byte(void)
{
    return (uint8_t)(rand64() & 0xFF);
}

// Generate random integer in range [0, max)
static int rand_range(int max)
{
    if (max <= 0) return 0;
    return (int)(rand64() % max);
}

// Seed URLs for mutation-based fuzzing
static const char *seed_urls[] = {
    "http://example.com",
    "https://user:pass@example.com:8080/path?query=value#fragment",
    "ftp://192.168.1.1/file.txt",
    "http://[2001:db8::1]/",
    "mailto:user@example.com",
    "file:///path/to/file",
    "http://example.com:80/path",
    "https://example.com/path%20with%20spaces",
    "http://localhost:3000",
    "http://127.0.0.1:8080/api/v1/resource",
    "//example.com/path",
    "/path/to/resource",
    "../relative/path",
    "?query=only",
    "#fragment-only",
    "http://example.com:99999",
    "http://256.256.256.256",
    "http://example..com",
    "http://:password@example.com",
    "http://user:@example.com",
    "http://example.com/path//double//slash",
    "http://example.com/path/../../../etc/passwd",
    "http://example.com/%2e%2e%2f",
    "http://example.com/\x00null",
    "http://example.com/\x0a\x0d",
    "http://\t\n\rexample.com",
    "http://example.com:0",
    "http://example.com:-1",
    "http://example.com:abc",
    "http://[::1]",
    "http://[::ffff:192.0.2.1]",
    "http://[2001:db8::1]:8080",
    "http://[::1",
    "http://::1]",
    "data:text/plain;base64,SGVsbG8=",
    "javascript:alert(1)",
    "about:blank",
};

#define NUM_SEEDS (sizeof(seed_urls) / sizeof(seed_urls[0]))

// Character classes for generation
static const char *char_classes[] = {
    "abcdefghijklmnopqrstuvwxyz",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    "0123456789",
    "://",
    "@.:/?#[]",
    "%",
    "\t\n\r ",
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08",
};

#define NUM_CHAR_CLASSES (sizeof(char_classes) / sizeof(char_classes[0]))

// Mutation strategies
typedef enum {
    MUT_BIT_FLIP,
    MUT_BYTE_FLIP,
    MUT_BYTE_INSERT,
    MUT_BYTE_DELETE,
    MUT_BYTE_REPLACE,
    MUT_CHUNK_INSERT,
    MUT_CHUNK_DELETE,
    MUT_CHUNK_DUPLICATE,
    MUT_INTERESTING_VALUE,
    MUT_COUNT
} MutationType;

// Interesting byte values
static const uint8_t interesting_bytes[] = {
    0x00, 0x01, 0xFF, 0x7F, 0x80,
    '/', '?', '#', ':', '@', '[', ']',
    '%', '.', '-', '_', '~',
};

// Mutate a buffer
static int mutate_buffer(char *dst, int dst_cap, const char *src, int src_len)
{
    if (src_len <= 0 || src_len >= dst_cap) {
        // Can't mutate empty or too large buffer
        return src_len;
    }

    // Copy source to destination
    memcpy(dst, src, src_len);
    int len = src_len;

    // Apply 1-5 random mutations
    int num_mutations = 1 + rand_range(5);
    for (int i = 0; i < num_mutations && len > 0 && len < dst_cap - 1; i++) {
        MutationType mut = rand_range(MUT_COUNT);
        int pos = rand_range(len);

        switch (mut) {
        case MUT_BIT_FLIP:
            if (pos < len) {
                dst[pos] ^= (1 << rand_range(8));
            }
            break;

        case MUT_BYTE_FLIP:
            if (pos < len) {
                dst[pos] ^= 0xFF;
            }
            break;

        case MUT_BYTE_INSERT:
            if (len < dst_cap - 1) {
                memmove(dst + pos + 1, dst + pos, len - pos);
                dst[pos] = rand_byte();
                len++;
            }
            break;

        case MUT_BYTE_DELETE:
            if (pos < len && len > 1) {
                memmove(dst + pos, dst + pos + 1, len - pos - 1);
                len--;
            }
            break;

        case MUT_BYTE_REPLACE:
            if (pos < len) {
                dst[pos] = rand_byte();
            }
            break;

        case MUT_CHUNK_INSERT: {
            int chunk_len = 1 + rand_range(16);
            if (len + chunk_len < dst_cap) {
                memmove(dst + pos + chunk_len, dst + pos, len - pos);
                for (int j = 0; j < chunk_len; j++) {
                    dst[pos + j] = rand_byte();
                }
                len += chunk_len;
            }
            break;
        }

        case MUT_CHUNK_DELETE: {
            int chunk_len = 1 + rand_range(16);
            if (pos + chunk_len <= len) {
                memmove(dst + pos, dst + pos + chunk_len, len - pos - chunk_len);
                len -= chunk_len;
            }
            break;
        }

        case MUT_CHUNK_DUPLICATE: {
            int chunk_len = 1 + rand_range(16);
            if (pos + chunk_len <= len && len + chunk_len < dst_cap) {
                memmove(dst + pos + chunk_len, dst + pos, len - pos);
                memcpy(dst + pos + chunk_len, dst + pos, chunk_len);
                len += chunk_len;
            }
            break;
        }

        case MUT_INTERESTING_VALUE:
            if (pos < len) {
                dst[pos] = interesting_bytes[rand_range(sizeof(interesting_bytes))];
            }
            break;

        default:
            break;
        }
    }

    return len;
}

// Generate a random URL-like string
static int generate_random_url(char *buf, int cap)
{
    int strategy = rand_range(3);

    if (strategy == 0) {
        // Fully random generation
        int len = rand_range(cap - 1);
        for (int i = 0; i < len; i++) {
            buf[i] = rand_byte();
        }
        return len;
    } else if (strategy == 1) {
        // Generate from character classes
        int len = rand_range(cap - 1);
        for (int i = 0; i < len; i++) {
            const char *class = char_classes[rand_range(NUM_CHAR_CLASSES)];
            int class_len = strlen(class);
            buf[i] = class[rand_range(class_len)];
        }
        return len;
    } else {
        // Mutation-based: pick a seed and mutate it
        const char *seed = seed_urls[rand_range(NUM_SEEDS)];
        int seed_len = strlen(seed);
        return mutate_buffer(buf, cap, seed, seed_len);
    }
}

// Fuzz url_parse
static void fuzz_url_parse(const char *input, int len)
{
    URL url;
    int flags_options[] = { 0, URL_FLAG_ALLOWREF, URL_FLAG_RFC3986, URL_FLAG_ALLOWREF | URL_FLAG_RFC3986 };
    int flags = flags_options[rand_range(4)];

    // Test with NULL cursor
    url_parse((char *)input, len, NULL, &url, flags);

    // Test with cursor
    int cursor = 0;
    url_parse((char *)input, len, &cursor, &url, flags);

    // Test serialization if parse succeeded
    if (url_parse((char *)input, len, NULL, &url, flags) == 0) {
        char output[4096];
        url_serialize(url, NULL, output, sizeof(output));
    }

    // Test percent decode on random substrings
    if (len > 0) {
        int start = rand_range(len);
        int end = start + rand_range(len - start);
        URL_String str = { (char *)input + start, end - start };
        char decoded[4096];
        url_percent_decode(str, decoded, sizeof(decoded));
    }

    // Test whitespace removal
    char tmp[4096];
    url_remove_white_space((char *)input, len, tmp, sizeof(tmp));
}

// Fuzz url_parse_ipv4
static void fuzz_ipv4_parse(const char *input, int len)
{
    URL_IPv4 ipv4;

    // Test with NULL cursor
    url_parse_ipv4((char *)input, len, NULL, &ipv4);

    // Test with cursor
    int cursor = 0;
    url_parse_ipv4((char *)input, len, &cursor, &ipv4);
}

// Fuzz url_parse_ipv6
static void fuzz_ipv6_parse(const char *input, int len)
{
    URL_IPv6 ipv6;

    // Test with NULL cursor
    url_parse_ipv6((char *)input, len, NULL, &ipv6);

    // Test with cursor
    int cursor = 0;
    url_parse_ipv6((char *)input, len, &cursor, &ipv6);
}

// Run one fuzz iteration
static void fuzz_iteration(int iteration)
{
    char input[4096];
    int len = generate_random_url(input, sizeof(input));

    // Ensure null termination for safety (though our API doesn't require it)
    if (len < (int)sizeof(input)) {
        input[len] = '\0';
    }

    // Fuzz different functions
    int target = rand_range(3);
    switch (target) {
    case 0:
        fuzz_url_parse(input, len);
        break;
    case 1:
        fuzz_ipv4_parse(input, len);
        break;
    case 2:
        fuzz_ipv6_parse(input, len);
        break;
    }
}

int main(int argc, char **argv)
{
    // Initialize RNG
    if (argc > 1) {
        rng_state = strtoull(argv[1], NULL, 10);
    } else {
        rng_state = time(NULL);
    }

    printf("URL Parser Fuzzer\n");
    printf("Seed: %llu\n", (unsigned long long)rng_state);
    printf("Running fuzz tests...\n\n");

    int num_iterations = 100000;
    if (argc > 2) {
        num_iterations = atoi(argv[2]);
    }

    clock_t start = clock();

    for (int i = 0; i < num_iterations; i++) {
        fuzz_iteration(i);

        // Progress update every 10000 iterations
        if (i > 0 && i % 10000 == 0) {
            double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
            double rate = i / elapsed;
            printf("\rIteration %d/%d (%.0f iter/sec)", i, num_iterations, rate);
            fflush(stdout);
        }
    }

    double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
    printf("\n\nCompleted %d iterations in %.2f seconds\n", num_iterations, elapsed);
    printf("Average rate: %.0f iterations/second\n", num_iterations / elapsed);
    printf("\nNo crashes detected!\n");

    return 0;
}
