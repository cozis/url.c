#include "url.h"

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

#ifdef NDEBUG
#define ASSERT(x) {}
#else
#define ASSERT(x) { if (!(x)) __builtin_trap(); }
#endif

#define UINT8_MAX  (URL_u8)  ((1 <<  8)-1)
#define UINT16_MAX (URL_u16) ((1 << 16)-1)

#define EMPTY (URL_String) { (void*) 0, 0 }
#define SLICE(src, off, end) (URL_String) { src + off, end - off }

static URL_b32 is_alpha(char c)
{
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z');
}

static URL_b32 is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static URL_b32 is_hex_digit(char c)
{
    return (c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F');
}

#if 0
static URL_b32 is_gen_delim(char c)
{
    return c == ':'
        || c == '/'
        || c == '?'
        || c == '#'
        || c == '['
        || c == ']'
        || c == '@';
}
#endif

static URL_b32 is_sub_delim(char c)
{
    return c == '!'
        || c == '$'
        || c == '&'
        || c == '\''
        || c == '('
        || c == ')'
        || c == '*'
        || c == '+'
        || c == ','
        || c == ';'
        || c == '=';
}

#if 0
static URL_b32 is_reserved(char c)
{
    return is_gen_delim(c)
        || is_sub_delim(c);
}
#endif

static URL_b32 is_unreserved(char c)
{
    return is_alpha(c)
        || is_digit(c)
        || c == '-'
        || c == '.'
        || c == '_'
        || c == '~';
}

static URL_b32 is_scheme_first(char c)
{
    return is_alpha(c);
}

static URL_b32 is_scheme(char c)
{
    return is_alpha(c)
        || is_digit(c)
        || c == '+'
        || c == '-'
        || c == '.';
}

static URL_b32 is_userinfo(char c)
{
    return is_unreserved(c)
        || is_sub_delim(c);
}

static URL_b32 is_reg_name(char c)
{
    return is_unreserved(c)
        || is_sub_delim(c);
}

static URL_b32 is_pchar(char c)
{
    return is_unreserved(c)
        || is_sub_delim(c)
        || c == ':'
        || c == '@';
}

static URL_b32 is_query(char c)
{
    return is_pchar(c)
        || c == '/'
        || c == '?';
}

static URL_b32 is_fragment(char c)
{
    return is_pchar(c)
        || c == '/'
        || c == '?';
}

// Returns 1 if a percent encoded character is
// at offset "cur" and 0 otherwise. If a '%'
// character is found but the following characters
// aren't hex, -1 is returned.
static int is_percent_encoded(char *src, int len, int cur)
{
    if (cur == len || src[cur] != '%')
        return 0;

    if (len - cur <= 2
        || !is_hex_digit(src[cur+1])
        || !is_hex_digit(src[cur+2]))
        return -1;

    return 1;
}

int url_parse_ipv4(char *src, int len, int *pcur, URL_IPv4 *out)
{
    int cur = pcur ? *pcur : 0;

    out->data = 0;
    for (int i = 0; i < 4; i++) {

        if (cur == len || !is_digit(src[cur]))
            return -1;

        URL_u8 byte = src[cur] - '0';
        cur++;

        while (cur < len && is_digit(src[cur])) {

            int n = src[cur] - '0';
            cur++;

            if (byte > (UINT8_MAX - n) / 10)
                return -1;
            byte = byte * 10 + n;
        }

        if (i < 3) {
            if (cur == len || src[cur] != '.')
                return -1;
            cur++;
        }

        out->data <<= 8;
        out->data |= byte;
    }

    if (pcur) *pcur = cur;
    return 0;
}

static int hex_digit_to_int(char c)
{
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return c - '0';
}

static int
parse_ipv6_comp(char *src, int len, int *pcur)
{
    int cur = pcur ? *pcur : 0;

	unsigned short buf;

	if (cur == len || !is_hex_digit(src[cur]))
		return -1;
	buf = hex_digit_to_int(src[cur]);
	cur++;

	if (cur == len || !is_hex_digit(src[cur]))
	    goto success;
	buf <<= 4;
	buf |= hex_digit_to_int(src[cur]);
	cur++;

	if (cur == len || !is_hex_digit(src[cur]))
	    goto success;
	buf <<= 4;
	buf |= hex_digit_to_int(src[cur]);
	cur++;

	if (cur == len || !is_hex_digit(src[cur]))
	    goto success;
	buf <<= 4;
	buf |= hex_digit_to_int(src[cur]);
	cur++;

success:
	if (pcur) *pcur = cur;
	return buf;
}

int url_parse_ipv6(char *src, int len, int *pcur, URL_IPv6 *out)
{
    int cur = pcur ? *pcur : 0;

    URL_u16 head[8];
	URL_u16 tail[8];
	int head_len = 0;
	int tail_len = 0;

	if (len - cur > 1
		&& src[cur+0] == ':'
		&& src[cur+1] == ':')
		cur += 2;
	else {
		for (;;) {

			int ret = parse_ipv6_comp(src, len, &cur);
			if (ret < 0)
			    return ret;

			head[head_len++] = (URL_u16) ret;
			if (head_len == 8)
			    break;

			if (cur == len || src[cur] != ':')
				return -1;
			cur++;

			if (cur < len && src[cur] == ':') {
				cur++;
				break;
			}
		}
	}

	if (head_len < 8) {
		while (cur < len && is_hex_digit(src[cur])) {

			int ret = parse_ipv6_comp(src, len, &cur);
			if (ret < 0)
			    return ret;

			tail[tail_len++] = (URL_u16) ret;
			if (head_len + tail_len == 8)
			    break;

			if (cur == len || src[cur] != ':')
				break;
			cur++;
		}
	}

	for (int i = 0; i < head_len; i++)
		out->data[i] = head[i];

	for (int i = 0; i < 8 - head_len - tail_len; i++)
		out->data[head_len + i] = 0;

	for (int i = 0; i < tail_len; i++)
		out->data[8 - tail_len + i] = tail[i];

	return 0;
}

int url_parse(char *src, int len, int *pcur, URL *out)
{
    int cur = pcur ? *pcur : 0;

    // Parse scheme characters until the end of the string,
    // a character not allowed in the scheme, or ':'. If the
    // found character wasn't ':', set the scheme to empty
    // and rollback the cursor.
    out->scheme = EMPTY;
    if (cur < len && is_scheme_first(src[cur])) {

        // Save the index of the first character of the scheme
        // and advance
        int off = cur;
        cur++;

        // Consume characters from the body of the scheme
        while (cur < len && is_scheme(src[cur]))
            cur++;

        if (cur == len || src[cur] != ':') {
            cur = 0; // Not a scheme after all
        } else {
            out->scheme = SLICE(src, off, cur);
            cur++; // Consume the ':'
        }
    }

    // If the following characters are // we expect an
    // authority. An authority consists of some optional
    // userinfo, a domain name, and a port.
    //
    //   scheme://user:pass@domain.com:port...
    //
    // We won't know whether the first portion is user
    // info or the domain until we hit a @ character or
    // a character not allowed in the user info, se we
    // need to backtrack in a similar way to the scheme
    URL_b32 authority = 0;
    if (len - cur > 1 && src[cur] == '/' && src[cur+1] == '/') {
        cur += 2; // Consume the //
        authority = 1;

        out->username = EMPTY;
        out->password = EMPTY;
        if (cur < len && (is_userinfo(src[cur]) || src[cur] == ':')) {

            int user_off = cur;

            while (cur < len) {
                if (is_userinfo(src[cur]))
                    cur++;
                else {
                    int ret = is_percent_encoded(src, len, cur);
                    if (ret < 0)
                        return ret;
                    if (ret == 0)
                        break;
                    cur += 3;
                }
            }

            if (cur < len && src[cur] == ':') {

                cur++; // Consume the ':'
                int pass_off = cur;

                while (cur < len) {
                    if (is_userinfo(src[cur]))
                        cur++;
                    else {
                        int ret = is_percent_encoded(src, len, cur);
                        if (ret < 0)
                            return ret;
                        if (ret == 0)
                            break;
                        cur += 3;
                    }
                }

                if (cur == len || src[cur] != '@') {
                    cur = user_off; // Not userinfo after all
                } else {
                    out->username = SLICE(src, user_off, pass_off-1);
                    out->password = SLICE(src, pass_off, cur);
                    cur++; // Consume the '@'
                }

            } else if (cur < len && src[cur] == '@') {
                out->username = SLICE(src, user_off, cur);
                out->password = EMPTY;
                 cur++; // Consume the '@'
            } else {
                cur = user_off; // Not userinfo
            }
        }

        // The domain may be a registered name, an IPv4 address,
        // or an IPv6 address.
        //   example.com
        //   127.0.0.1
        //   [abcd:abcd::0001]
        // Note that an IPv4 could technically be parsed as a
        // registered name, so it's important that we first try
        // the IPv4 rule, and then the registered name rule.
        int host_off = cur;
        if (cur < len && src[cur] == '[') {
            cur++; // Consume the '['

            // We started with an '[', so it's definitely an IPv6
            out->host_type = URL_HOST_IPV6;

            int ret = url_parse_ipv6(src, len, &cur, &out->host_ipv6);
            if (ret < 0)
                return ret;

            if (cur == len || src[cur] != ']')
                return -1; // Missing ']' after IPv6
            cur++;

        } else {

            URL_b32 is_ipv4 = 0;

            // First, try the IPv4 rule
            if (is_digit(src[cur])) {
                if (url_parse_ipv4(src, len, &cur, &out->host_ipv4) == 0) {
                    out->host_type = URL_HOST_IPV4;
                    is_ipv4 = 1;
                }
            }

            if (!is_ipv4) {
                if (cur < len && (is_reg_name(src[cur]) || is_percent_encoded(src, len, cur) == 1)) {

                    int ret = is_percent_encoded(src, len, cur);
                    ASSERT(ret >= 0);
                    cur += (ret == 1) ? 3 : 1;

                    out->host_type = URL_HOST_NAME;

                    while (cur < len) {
                        if (is_reg_name(src[cur]))
                            cur++;
                        else {
                            int ret = is_percent_encoded(src, len, cur);
                            if (ret < 0)
                                return ret;
                            if (ret == 0)
                                break;
                            cur += 3;
                        }
                    }

                } else {
                    out->host_type = URL_HOST_EMPTY;
                }
            }
        }
        out->host_text = SLICE(src, host_off, cur);

        // Now we parse the port
        out->no_port = 1;
        out->port    = 0;
        if (cur < len && src[cur] == ':') {

            cur++; // Consume the ':'
            out->no_port = 0;

            while (cur < len && is_digit(src[cur])) {

                int n = src[cur] - '0';
                cur++;

                if (out->port > (UINT16_MAX - n) / 10)
                    return -1; // Overflow
                out->port = out->port * 10 + n;
            }
        }
    } else {
        out->username = EMPTY;
        out->password = EMPTY;
        out->host_text = EMPTY;
        out->host_type = URL_HOST_EMPTY;
        out->port = 0;
        out->no_port = 1;
    }

    // Now we parse the path. This is most difficult
    // component as it changes based on what comes
    // before it.
    //
    // If an URL contains an authority component, it
    // must be absolute or empty.
    if (authority && (cur == len || src[cur] != '/')) {
        out->path = EMPTY;
    } else {

        int off = cur;

        while (cur < len) {
            if (is_pchar(src[cur]) || src[cur] == '/')
                cur++;
            else {
                int ret = is_percent_encoded(src, len, cur);
                if (ret < 0)
                    return ret;
                if (ret == 0)
                    break;
                cur += 3;
            }
        }

        out->path = SLICE(src, off, cur);
    }

    // Consume the query
    out->query = EMPTY;
    if (cur < len && src[cur] == '?') {

        cur++; // Consume '?'
        int off = cur;

        while (cur < len) {
            if (is_query(src[cur]))
                cur++;
            else {
                int ret = is_percent_encoded(src, len, cur);
                if (ret < 0)
                    return ret;
                if (ret == 0)
                    break;
                cur += 3;
            }
        }

        out->query = SLICE(src, off, cur);
    }

    // Consume the fragment
    out->fragment = EMPTY;
    if (cur < len && src[cur] == '#') {

        cur++; // Consume '#'
        int off = cur;

        while (cur < len) {
            if (is_fragment(src[cur]))
                cur++;
            else {
                int ret = is_percent_encoded(src, len, cur);
                if (ret < 0)
                    return ret;
                if (ret == 0)
                    break;
                cur += 3;
            }
        }

        out->fragment = SLICE(src, off, cur);
    }

    // If a cursor pointer was provided, it is assumed
    // the caller is parsing a string that contains an
    // URL and may be followed by something else, therefore
    // we allow partially parsing the source. If no cursor
    // pointer was provided, we expect the source to be
    // an exact URL string.
    if (pcur) {
        *pcur = cur;
    } else {
        if (cur != len)
            return -1;
    }
    return 0;
}
