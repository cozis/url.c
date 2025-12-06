#include <url.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void)
{
    char url[] = "http://example.com";

    // This is how you parse an URL
    URL parsed_url;
    int ret = url_parse(url, strlen(url), NULL, &parsed_url, 0);
    if (ret < 0) {
        printf("Invalid URL!\n");
        return -1;
    }

    // Now the URL's component are stored in the URL
    // structure. Note that the fields of URL are slices
    // into the source string.

    // Let's print the URL's domain. Note that any
    // field except for the scheme may be percent-encoded,
    // so if you want the raw representation of a
    // field you should decode it:
    char domain[1<<9];
    int domain_len = url_percent_decode(parsed_url.host_text, domain, sizeof(domain));

    // url_percent_decode may fail if the percent-encoding
    // is invalid or the buffer is too small. The url_parse
    // function validates the percent-encoding, so we only
    // need to worry about the buffer size.
    assert(domain_len > -1);
    if (domain_len >= (int) sizeof(domain)) {
        printf("Domain buffer is too small\n");
        return -1;
    }

    // Note that url.c never adds null characters to outputs
    domain[domain_len] = '\0';

    printf("The domain is: %s\n", domain);
    return 0;
}
