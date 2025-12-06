#ifndef URL_INCLUDED
#define URL_INCLUDED
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <https://unlicense.org>

#define URL_STATIC_ASSERT _Static_assert

typedef unsigned char  URL_u8;
typedef unsigned short URL_u16;
typedef unsigned int   URL_u32;
typedef unsigned int   URL_b32;

URL_STATIC_ASSERT(sizeof(URL_u8)  == 1, "");
URL_STATIC_ASSERT(sizeof(URL_u16) == 2, "");
URL_STATIC_ASSERT(sizeof(URL_u32) == 4, "");
URL_STATIC_ASSERT(sizeof(URL_b32) == 4, "");

typedef struct {
    char *ptr;
    int   len;
} URL_String;

typedef enum {
    URL_HOST_EMPTY,
    URL_HOST_IPV4,
    URL_HOST_IPV6,
    URL_HOST_NAME,
} URL_HostType;

typedef struct {
    URL_u32 data;
} URL_IPv4;

typedef struct {
    URL_u16 data[8];
} URL_IPv6;

URL_STATIC_ASSERT(sizeof(URL_IPv4) == 4, "");
URL_STATIC_ASSERT(sizeof(URL_IPv6) == 16, "");

typedef struct {

    URL_b32 no_authority;
    URL_b32 no_userinfo;

    // May be empty
    URL_String scheme;

    // Both may be empty
    URL_String username;
    URL_String password;

    // The raw host string is stored in host_text.
    // If the host is an IPv4 or IPv6, its parsed
    // value is also stored in host_ipv4 or host_ipv6
    // (host byte order).
    // Note that the host may be empty, in which
    // case host_type=URL_HOST_EMPTY.
    URL_HostType host_type;
    URL_String   host_text;
    union {
        URL_IPv4 host_ipv4;
        URL_IPv6 host_ipv6;
    };

    // If no port was specified, no_port is set
    // to 1 and port to 0.
    URL_b32 no_port;
    URL_u16 port;

    URL_String path;

    // May be empty
    URL_String query;

    // May be empty
    URL_String fragment;

} URL;

int url_parse_ipv4(char *src, int len, int *pcur, URL_IPv4 *out);
int url_parse_ipv6(char *src, int len, int *pcur, URL_IPv6 *out);

int url_parse(char *src, int len, int *pcur, URL_b32 strict, URL *out);

int url_resolve_reference(char *src, int len, int *pcur,
    URL *base, URL_b32 strict, char *dst, int cap);

int url_remove_white_space(char *src, int len, char *dst, int cap);

int url_decode_field(URL_String field, char *dst, int cap);

#endif // URL_INCLUDED
