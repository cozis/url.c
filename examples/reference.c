#include <stdio.h>
#include <string.h>
#include "../url.h"

int main(void)
{
    char base[] = "http://website.com/users/000";
    char ref[] = "../images/cat.png";

    URL parsed_base;
    int ret = url_parse(base, strlen(base), NULL, 1, &parsed_base);
    if (ret < 0) {
        printf("Couldn't parse base URL\n");
        return -1;
    }

    char dst[1<<10];
    ret = url_resolve_reference(ref, strlen(ref), NULL, &parsed_base, 1, dst, (int) sizeof(dst));
    if (ret < 0) {
        printf("Couldn't resolve reference\n");
        return -1;
    }

    // http://website.com/images/cat.png
    printf("%.*s\n", ret, dst);
    return 0;
}
