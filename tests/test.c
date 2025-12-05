#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <stdbool.h>

#include "cJSON.h"
#include "../url.h"

#define BLACK   "\033[1;30m"
#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define WHITE   "\033[1;37m"
#define RESET   "\033[1;0m"

static char test_output_buffer[1<<13];
static int  test_output_buffer_used = 0;

void test_printf(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(
        test_output_buffer         + test_output_buffer_used,
        sizeof(test_output_buffer) - test_output_buffer_used,
        fmt, args);
    va_end(args);
    test_output_buffer_used += len;
}

void test_print_str(URL_String str)
{
    bool last_char_was_unprintable = false;
    for (int i = 0; i < str.len; i++) {

        uint8_t c = (uint8_t) str.ptr[i];
        if (c > 32 && c <= 127) {
            if (last_char_was_unprintable)
                test_printf(" ");
            test_printf("%c", c);
            last_char_was_unprintable = false;
        } else {
            test_printf(" ");
            if (c == '\t') {
                test_printf(YELLOW"\\t"RESET);
            } else if (c == '\n') {
                test_printf(YELLOW"\\n"RESET);
            } else if (c == '\r') {
                test_printf(YELLOW"\\r"RESET);
            } else if (c == ' ') {
                test_printf(YELLOW"SP"RESET);
            } else {
                static const char table[] = "0123456789abcdef";
                test_printf(YELLOW"%c%c"RESET, table[c >> 4], table[c & 0xF]);
            }
            last_char_was_unprintable = true;
        }
    }
}

static int get_string_field(cJSON *json, char *key, URL_String *out)
{
    cJSON *child = cJSON_GetObjectItemCaseSensitive(json, key);
    if (child == NULL || !cJSON_IsString(child))
        return -1;
    *out = (URL_String) {
        child->valuestring,
        strlen(child->valuestring)
    };
    return 0;
}

static void test_nested_json_dump(cJSON *json)
{
    if (cJSON_IsString(json)) {
        test_printf("\"%s\"", json->valuestring);
    } else if (cJSON_IsFalse(json)) {
        test_printf("false");
    } else if (cJSON_IsTrue(json)) {
        test_printf("true");
    } else if (cJSON_IsNull(json)) {
        test_printf("null");
    } else if (cJSON_IsNumber(json)) {
        test_printf("%f", json->valuedouble);
    } else if (cJSON_IsArray(json)) {
        test_printf("[ ... ]");
    } else if (cJSON_IsObject(json)) {
        test_printf("{ ... }");
    } else {
        test_printf("???");
    }
}

static void test_json_dump(cJSON *json)
{
    if (!cJSON_IsObject(json)) {
        test_printf(YELLOW"    ");
        test_nested_json_dump(json);
        test_printf("\n"RESET);
        return;
    }
    test_printf(YELLOW"    {\n");

    cJSON *child = json->child;
    while (child) {
        test_printf("      \"%s\": ", child->string);
        test_nested_json_dump(child);
        child = child->next;
        if (child)
            test_printf(",");
        test_printf("\n");
    }
    test_printf("    }\n"RESET);
}

static int json_to_url(cJSON *json, URL *url)
{
    // TODO: Only a subset of fields is set here

    if (get_string_field(json, "protocol", &url->scheme) < 0) {
        test_printf("  JSON object field \"protocol\" is missing or is not a string:\n\n");
        test_json_dump(json);
        return -1;
    }

    if (get_string_field(json, "username", &url->username) < 0) {
        test_printf("  JSON object field \"username\" is missing or is not a string:\n\n");
        test_json_dump(json);
        return -1;
    }

    if (get_string_field(json, "password", &url->password) < 0) {
        test_printf("  JSON object field \"password\" is missing or is not a string:\n\n");
        test_json_dump(json);
        return -1;
    }

    if (get_string_field(json, "hostname", &url->host_text) < 0) {
        test_printf("  JSON object field \"hostname\" is missing or is not a string:\n\n");
        test_json_dump(json);
        return -1;
    }

    URL_String port;
    if (get_string_field(json, "port", &port) < 0) {
        test_printf("  JSON object field \"port\" is missing or is not a string:\n\n");
        test_json_dump(json);
        return -1;
    }

    if (get_string_field(json, "pathname", &url->path) < 0) {
        test_printf("  JSON object field \"pathname\" is missing or is not a string:\n\n");
        test_json_dump(json);
        return -1;
    }

    if (get_string_field(json, "search", &url->query) < 0) {
        test_printf("  JSON object field \"search\" is missing or is not a string:\n\n");
        test_json_dump(json);
        return -1;
    }

    if (get_string_field(json, "hash", &url->fragment) < 0) {
        test_printf("  JSON object field \"hash\" is missing or is not a string:\n\n");
        test_json_dump(json);
        return -1;
    }

    if (url->scheme.len > 0) {
        if (url->scheme.ptr[url->scheme.len-1] != ':') {
            test_printf(" JSON object's scheme field doesn't end with ':' as expected\n");
            return -1;
        }
        url->scheme.len--;
    }

    if (url->query.len > 0) {
        if (url->query.ptr[0] != '?') {
            test_printf(" JSON object's query field doesn't start with '?' as expected\n");
            return -1;
        }
        url->query.ptr++;
        url->query.len--;
    }

    if (url->fragment.len > 0) {
        if (url->fragment.ptr[0] != '#') {
            test_printf(" JSON object's fragment field doesn't start with '#' as expected\n");
            return -1;
        }
        url->fragment.ptr++;
        url->fragment.len--;
    }

    char tmp[8];
    if (port.len >= (int) sizeof(tmp)) {
        test_printf(" JSON object's port field is longer than expected\n");
        return -1;
    }
    memcpy(tmp, port.ptr, port.len);
    tmp[port.len] = 0;

    url->port = atoi(tmp);
    return 0;
}

static bool streq(URL_String a, URL_String b)
{
    if (a.len != b.len)
        return false;
    return !memcmp(a.ptr, b.ptr, a.len);
}

static int compare_urls(URL url_1, URL url_2, char *label_1, char *label_2)
{
    #define UNPACK(s) (s).len, (s).ptr

    bool ok = true;

    if (!streq(url_1.scheme, url_2.scheme)) {
        if (ok) test_printf("\n");
        test_printf("  scheme mismatch\n");
        test_printf("    %s [%.*s]\n", label_1, UNPACK(url_1.scheme));
        test_printf("    %s [%.*s]\n", label_2, UNPACK(url_2.scheme));
        ok = false;
    }

    if (!streq(url_1.username, url_2.username)) {
        if (ok) test_printf("\n");
        test_printf("  username mismatch\n");
        test_printf("    %s [%.*s]\n", label_1, UNPACK(url_1.username));
        test_printf("    %s [%.*s]\n", label_2, UNPACK(url_2.username));
        ok = false;
    }

    if (!streq(url_1.password, url_2.password)) {
        if (ok) test_printf("\n");
        test_printf("  password mismatch\n");
        test_printf("    %s [%.*s]\n", label_1, UNPACK(url_1.password));
        test_printf("    %s [%.*s]\n", label_2, UNPACK(url_2.password));
        ok = false;
    }

    if (!streq(url_1.host_text, url_2.host_text)) {
        if (ok) test_printf("\n");
        test_printf("  host mismatch\n");
        test_printf("    %s [%.*s]\n", label_1, UNPACK(url_1.host_text));
        test_printf("    %s [%.*s]\n", label_2, UNPACK(url_2.host_text));
        ok = false;
    }

    if (url_1.port != url_2.port) {
        if (ok) test_printf("\n");
        test_printf("  port mismatch\n");
        test_printf("    %s [%d]\n", label_1, url_1.port);
        test_printf("    %s [%d]\n", label_2, url_2.port);
        ok = false;
    }

    if (!streq(url_1.path, url_2.path)) {
        if (ok) test_printf("\n");
        test_printf("  path mismatch\n");
        test_printf("    %s [%.*s]\n", label_1, UNPACK(url_1.path));
        test_printf("    %s [%.*s]\n", label_2, UNPACK(url_2.path));
        ok = false;
    }

    if (!streq(url_1.query, url_2.query)) {
        if (ok) test_printf("\n");
        test_printf("  query mismatch\n");
        test_printf("    %s [%.*s]\n", label_1, UNPACK(url_1.query));
        test_printf("    %s [%.*s]\n", label_2, UNPACK(url_2.query));
        ok = false;
    }

    if (!streq(url_1.fragment, url_2.fragment)) {
        if (ok) test_printf("\n");
        test_printf("  fragment mismatch\n");
        test_printf("    %s [%.*s]\n", label_1, UNPACK(url_1.fragment));
        test_printf("    %s [%.*s]\n", label_2, UNPACK(url_2.fragment));
        ok = false;
    }

    return (int) (ok ? 1 : 0);
}

static int run_test(cJSON *json)
{
    cJSON *json_input = cJSON_GetObjectItemCaseSensitive(json, "input");
    if (json_input == NULL || !cJSON_IsString(json_input)) {
        test_printf("  JSON obect is missing the \"input\" field or it's not a string:\n");
        test_printf("\n");
        test_json_dump(json);
        return -1;
    }
    URL_String src = { json_input->valuestring, strlen(json_input->valuestring) };

    cJSON *json_base = cJSON_GetObjectItemCaseSensitive(json, "base");
    if (json_base == NULL) {
        test_printf("  JSON obect is missing the \"base\" field:\n");
        test_printf("\n");
        test_json_dump(json);
        return -1;
    }

    int ret;
    char tmp[1<<10];
    char buf[1<<10];
    URL parsed;
    if (cJSON_IsNull(json_base)) {
        ret = url_parse(src.ptr, src.len, NULL, false, &parsed);
    } else {
        if (!cJSON_IsString(json_base)) {
            test_printf("  JSON object field \"base\" is not a string or null:\n");
            test_printf("\n");
            test_json_dump(json);
            return -1;
        }
        URL_String base = { json_base->valuestring, strlen(json_base->valuestring) };

        URL parsed_base;
        ret = url_parse(base.ptr, base.len, NULL, false, &parsed_base);
        if (ret < 0) {
            test_printf("\n");
            test_printf("  JSON object has an invalid \"base\" field:\n");
            test_printf("    base: ");
            test_print_str(base);
            test_printf("\n");
            return -1;
        }
        assert(ret == 0);

        ret = url_remove_white_space(src.ptr, src.len, tmp, (int) sizeof(tmp));
        if (ret > (int) sizeof(tmp)) {
            assert(0); // TODO
        }
        src.ptr = tmp;
        src.len = ret;

        ret = url_resolve_reference(src.ptr, src.len, NULL, &parsed_base, false, buf, (int) sizeof(buf));
        if (ret >= (int) sizeof(buf)) {
            test_printf("\n");
            test_printf("  Resolve relative reference is too long for the static buffer\n");
            test_printf("    base: ");
            test_print_str(base);
            test_printf("\n");
            test_printf("    input: ");
            test_print_str(src);
            test_printf("\n");
            return -1;
        }

        if (ret > -1)
            ret = url_parse(buf, ret, NULL, false, &parsed);
    }

    // Failing test cases have a "failure: true" field
    cJSON *failure = cJSON_GetObjectItemCaseSensitive(json, "failure");
    if (failure != NULL) {
        // If a failure field is present, we expect it to be true
        if (!cJSON_IsTrue(failure)) {
            test_printf("\n");
            test_printf("  JSON object has the \"failure\" field but it's not \"true\"\n");
            return -1;
        }
    }

    if (failure == NULL) {

        if (ret < 0) {
            test_printf("\n");
            test_printf("  Parsing was expected to succede but failed\n");
            test_printf("    input: ");
            test_print_str(src);
            test_printf("\n");
            return 0;
        }

        URL expected;
        if (json_to_url(json, &expected) < 0)
            return -1;

        if (!compare_urls(parsed, expected, "parsed", "expected")) {
            test_printf("  input: ");
            test_print_str(src);
            test_printf("\n");
            return 0;
        }

        test_printf(" ");
        test_print_str(src);
        test_printf("\n");
        return 1;

    } else {

        if (ret == 0) {
            test_printf("\n");
            test_printf("  Parsing was expected to fail but succeded\n");
            test_printf("\n");
            test_json_dump(json);
            return 0;
        }
        if (ret != -1) {
            test_printf("\n");
            test_printf("  Was expected a return value of -1 on error but %d was returned\n", ret);
            return 0;
        }

        test_printf("\n");
        return 1;
    }
}

static char *load_file(char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
        return NULL;

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = malloc(len+1);
    if (buf == NULL) {
        fclose(fp);
        return NULL;
    }

    fread(buf, 1, len+1, fp);
    if (ferror(fp) || !feof(fp)) {
        free(buf);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    buf[len] = '\0';
    return buf;
}

int main(void)
{
    char *path = "wpt/urltestdata.json";
    char *buf = load_file(path);
    if (buf == NULL) {
        fprintf(stderr, "Error: Couldn't open %s\n", path);
        return -1;
    }

    cJSON *root = cJSON_Parse(buf);
    if (root == NULL) {
        fprintf(stderr, "Error: %s doesn't contain valid JSON", path);
        free(buf);
        return -1;
    }

    if (!cJSON_IsArray(root)) {
        fprintf(stderr, "Error: %s doesn't contain a JSON array as expected", path);
        free(buf);
        return -1;
    }

    int num_failed = 0;
    int num_passed = 0;
    int num_aborted = 0;
    int num_tests = 0;
    int last_test_result = 0;
    for (cJSON *child = root->child; child;
        child = child->next) {

        if (!cJSON_IsObject(child))
            continue;

        test_output_buffer_used = 0;

        int ret = run_test(child);
        switch (ret) {
        case 0:
            printf("\n");
            printf(RED"Test %d: FAILED"RESET, num_tests);
            num_failed++;
            // Test failed
            break;
        case 1:
            if (last_test_result != 1)
                printf("\n");
            printf(GREEN"Test %d: PASSED"RESET, num_tests);
            num_passed++;
            // Test passed
            break;
        case -1:
            printf("\n");
            printf(MAGENTA"Test %d: ABORTED"RESET, num_tests);
            num_aborted++;
            // Test aborted
            break;
        default:
            // Unreachable
            break;
        }
        last_test_result = ret;
        num_tests++;
        fwrite(test_output_buffer, 1, test_output_buffer_used, stdout);
    }

    printf("Results:\n");
    printf("  passed : %d\n", num_passed);
    printf("  failed : %d\n", num_failed);
    printf("  aborted: %d\n", num_aborted);

    free(buf);
    return 0;
}
