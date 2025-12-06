#include <url.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void)
{
    // We can translate parsed URLs back into strings
    // using the url_serialize function

    char url[] = "http://example.com";

    URL parsed_url;
    int ret = url_parse(url, strlen(url), NULL, &parsed_url, 0);
    if (ret < 0) {
        printf("Invalid URL!\n");
        return -1;
    }

    char buf[1<<9];
    ret = url_serialize(parsed_url, NULL, buf, sizeof(buf));

    // On error, url_serialize returns -1. Errors can
    // only occur when relative references are involved,
    // which isn't our case.
    assert(ret > -1);

    // We still need to worry about the buffer's capacity
    // though.
    if (ret >= (int) sizeof(buf)) {
        printf("Serialization buffer is too small\n");
        return -1;
    }

    // url.c never adds null terminators
    buf[ret] = '\0';

    printf("Serialized URL: %s\n", buf);
    return 0;
}
