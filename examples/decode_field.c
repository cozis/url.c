#include <stdio.h>
#include <string.h>
#include "../url.h"

int main(void)
{
    char src[] = "http://web%73ite.com";

    URL parsed;
    int ret = url_parse(src, strlen(src), NULL, 1, &parsed);
    if (ret < 0) {
        printf("Couldn't parse base URL\n");
        return -1;
    }

    printf("%.*s\n",
        parsed.host_text.len,
        parsed.host_text.ptr);

    char buf[100];
    ret = url_decode_field(parsed.host_text, buf, (int) sizeof(buf)-1);
    if (ret < 0 || ret >= (int) sizeof(buf)) {
        printf("Couldn't decode field\n");
        return -1;
    }

    printf("%.*s\n", ret, buf);
    return 0;
}
