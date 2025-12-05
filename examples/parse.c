#include <stdio.h>
#include <string.h>
#include "../url.h"

int main(void)
{
    char src[] = "http://website.com/users/000?filter=no#title";

    URL parsed;
    int ret = url_parse(src, strlen(src), NULL, 1, &parsed);
    if (ret < 0) {
        printf("Couldn't parse base URL\n");
        return -1;
    }

    // http
    printf("scheme=%.*s\n",
        parsed.scheme.len,
        parsed.scheme.ptr);

    // (empty)
    if (parsed.username.len == 0)
        printf("username=(empty)\n");
    else
        printf("username=%.*s\n",
            parsed.username.len,
            parsed.username.ptr);

    // (password)
    if (parsed.password.len == 0)
        printf("password=(empty)\n");
    else
        printf("password=%.*s\n",
            parsed.password.len,
            parsed.password.ptr);

    if (parsed.host_type == URL_HOST_EMPTY)
        printf("host=(empty)\n");
    else
        printf("host=%.*s\n",
            parsed.host_text.len,
            parsed.host_text.ptr);

    if (parsed.no_port)
        printf("port=(empty)\n");
    else
        printf("port=%d\n", parsed.port);

    printf("path=%.*s\n",
        parsed.path.len,
        parsed.path.ptr);

    printf("query=%.*s\n",
        parsed.query.len,
        parsed.query.ptr);

    printf("fragment=%.*s\n",
        parsed.fragment.len,
        parsed.fragment.ptr);

    return 0;
}
