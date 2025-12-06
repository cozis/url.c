#include <url.h>
#include <stdio.h>

int main(void)
{
    // We can also parse URLs when they are part of a larger
    // string. This is useful when we know the starting offset
    // of an URL but don't know the end.
    //
    // For instance, say we have this string:
    char str[] = "   http://websiteA.com    http://websiteB.com/index.html  ";
    int  len = sizeof(str)-1;

    // The "pcur" argument of url_parse allows it to tell us
    // how long the URL is.

    int cur = 0;
    for (;;) {

        // Skip whitespace preceding the URL
        while (cur < len && str[cur] == ' ')
            cur++;

        if (cur == len)
            break; // No more string

        int url_off = cur;

        URL url;
        int ret = url_parse(str, len, &cur, &url, 0);
        if (ret < 0) {
            printf("Invalid URL\n");
            return -1;
        }

        int url_len = cur - url_off;

        // Thanks to the pcur argument of url_parse we can infer
        // when the URL ends. We can get the original text of the
        // URL my slicing the source string using the starting and
        // end offsets, or use something like url_serialize to
        // translate the URL structure into a string.
        printf("Found an URL: %.*s\n", url_len, str + url_off);
    }

    return 0;
}
