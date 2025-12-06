#include <url.h>
#include <stdio.h>

int main(void)
{
    URL_Builder builder;
    url_builder_init(&builder);

    url_builder_set_scheme(&builder, "http", -1);

    url_builder_set_username(&builder, "cozis", -1);

    url_builder_set_password(&builder, "my_secret", -1);

    url_builder_set_host_name(&builder, "www.ex@mple.com", -1);

    url_builder_set_path(&builder, "/../../index.html", -1);

    char url[1<<9];
    int len = url_builder_finalize(&builder, url, sizeof(url));
    if (len < 0) {
        printf("Couldn't build URL\n");
        return -1;
    }
    if (len >= (int) sizeof(url)) {
        printf("URL buffer is too small\n");
        return -1;
    }
    url[len] = '\0';

    printf("Built URL: %s\n", url);
    return 0;
}
