#include <stdio.h>
#include <string.h>
#include "url.h"

#define UNPACK(s) (s).len, (s).ptr

static char *host_type_to_str(URL_HostType type)
{
    switch (type) {
    case URL_HOST_EMPTY:
        return "";
    case URL_HOST_IPV4:
        return "IPv4";
    case URL_HOST_IPV6:
        return "IPv6";
    case URL_HOST_NAME:
        return "name";
    default:
    }
    return "???";
}

static void dump_parsed_url(URL *parsed)
{
    printf(
        "URL {\n"
        "  scheme   = [%.*s]\n"
        "  username = [%.*s]\n"
        "  password = [%.*s]\n"
        "  host     = [%.*s] (%s)\n"
        "  port     = [%d]%s\n"
        "  path     = [%.*s]\n"
        "  query    = [%.*s]\n"
        "  fragment = [%.*s]\n"
        "}\n",
        UNPACK(parsed->scheme),
        UNPACK(parsed->username),
        UNPACK(parsed->password),
        UNPACK(parsed->host_text),
        host_type_to_str(parsed->host_type),
        parsed->port,
        parsed->no_port ? " (empty)" : "",
        UNPACK(parsed->path),
        UNPACK(parsed->query),
        UNPACK(parsed->fragment));
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Missing URL\n");
        return -1;
    }
    char *url = argv[1];

    URL parsed;
    int ret = url_parse(url, strlen(url), NULL, &parsed);

    printf("ret=%d\n", ret);
    if (ret == 0)
        dump_parsed_url(&parsed);
    return 0;
}
