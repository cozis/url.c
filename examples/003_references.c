#include <url.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    // url.c also allows us to parse relative references to URLs.
    // These are strings that aren't technically URLs but may be
    // evaluated to one in reference to a base URL.
    //
    // Here's an example:
    char base_url[] = "http://example.com/files/document.txt";
    char relative_reference[] = "../images/cat.png";

    // The url_serialize function allows us to translate the
    // reference into an URL. But first, we need to parse both
    // the base URL and reference.

    URL parsed_base_url;
    int ret = url_parse(base_url, strlen(base_url), NULL, &parsed_base_url, 0);
    if (ret < 0) {
        printf("Invalid base URL\n");
        return -1;
    }

    // Note that url_parse will reject relative references by
    // default. We need to pass the URL_FLAG_ALLOWREF flag.
    URL parsed_relative_reference;
    ret = url_parse(relative_reference, strlen(relative_reference),
        NULL, &parsed_relative_reference, URL_FLAG_ALLOWREF);
    if (ret < 0) {
        printf("Invalid relative reference\n");
        return -1;
    }

    // Now we can resolve the reference by serializing it with
    // the base URL
    char buf[1<<9];
    ret = url_serialize(parsed_relative_reference, &parsed_base_url, buf, sizeof(buf));

    // Since url_serialize was called with a non-NULL base URL
    // argument, it may fail. We need to check for a negative
    // return value.
    if (ret < 0) {
        printf("Reference resolution failed\n");
        return -1;
    }

    // Check that the buffer's capacity was enough
    if (ret >= (int) sizeof(buf)) {
        printf("Serialization buffer is too small\n");
        return -1;
    }

    // All good. Now we can print the result
    buf[ret] = '\0';
    printf("Resolved reference: %s\n", buf);

    return 0;
}
