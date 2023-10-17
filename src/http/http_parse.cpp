#include "../headers.h"

#include "http.h"
#include "http_parse.h"

extern Cycle *cyclePtr;

uint32_t usual[] = {
    0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
    0x7fff37d6, /* 0111 1111 1111 1111  0011 0111 1101 0110 */

    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
    0x7fffffff, /* 0111 1111 1111 1111  1111 1111 1111 1111 */

    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */
    0xffffffff  /* 1111 1111 1111 1111  1111 1111 1111 1111 */
};

int parseRequestLine(std::shared_ptr<Request> r)
{
    // LOG_INFO << "parse request line";
    u_char c, ch, *p, *m;

    auto &state = r->requestState;
    auto &buffer = r->c->readBuffer_;

    for (p = buffer.now->start + buffer.now->pos; p < buffer.now->start + buffer.now->len; p++)
    {
        ch = *p;

        switch (state)
        {

        /* HTTP methods: GET, HEAD, POST */
        case RequestState::sw_start:
            r->request_start = p;

            if (ch == CR || ch == LF)
            {
                break;
            }

            if ((ch < 'A' || ch > 'Z') && ch != '_' && ch != '-')
            {
                return ERROR;
            }

            state = RequestState::sw_method;
            break;

        case RequestState::sw_method:
            if (ch == ' ')
            {
                r->method_end = p - 1;
                m = r->request_start;

                switch (p - m)
                {

                case 3:
                    if (ngx_str3_cmp(m, 'G', 'E', 'T', ' '))
                    {
                        r->method = Method::GET;
                        break;
                    }

                    if (ngx_str3_cmp(m, 'P', 'U', 'T', ' '))
                    {
                        r->method = Method::PUT;
                        break;
                    }

                    break;

                case 4:
                    if (m[1] == 'O')
                    {

                        if (ngx_str3Ocmp(m, 'P', 'O', 'S', 'T'))
                        {
                            r->method = Method::POST;
                            break;
                        }

                        if (ngx_str3Ocmp(m, 'C', 'O', 'P', 'Y'))
                        {
                            r->method = Method::COPY;
                            break;
                        }

                        if (ngx_str3Ocmp(m, 'M', 'O', 'V', 'E'))
                        {
                            r->method = Method::MOVE;
                            break;
                        }

                        if (ngx_str3Ocmp(m, 'L', 'O', 'C', 'K'))
                        {
                            r->method = Method::LOCK;
                            break;
                        }
                    }
                    else
                    {

                        if (ngx_str4cmp(m, 'H', 'E', 'A', 'D'))
                        {
                            r->method = Method::HEAD;
                            break;
                        }
                    }

                    break;

                case 5:
                    if (ngx_str5cmp(m, 'M', 'K', 'C', 'O', 'L'))
                    {
                        r->method = Method::MKCOL;
                        break;
                    }

                    if (ngx_str5cmp(m, 'P', 'A', 'T', 'C', 'H'))
                    {
                        r->method = Method::PATCH;
                        break;
                    }

                    if (ngx_str5cmp(m, 'T', 'R', 'A', 'C', 'E'))
                    {
                        r->method = Method::TRACE;
                        break;
                    }

                    break;

                case 6:
                    if (ngx_str6cmp(m, 'D', 'E', 'L', 'E', 'T', 'E'))
                    {
                        r->method = Method::DELETE;
                        break;
                    }

                    if (ngx_str6cmp(m, 'U', 'N', 'L', 'O', 'C', 'K'))
                    {
                        r->method = Method::UNLOCK;
                        break;
                    }

                    break;

                case 7:
                    if (ngx_str7_cmp(m, 'O', 'P', 'T', 'I', 'O', 'N', 'S', ' '))
                    {
                        r->method = Method::OPTIONS;
                    }

                    if (ngx_str7_cmp(m, 'C', 'O', 'N', 'N', 'E', 'C', 'T', ' '))
                    {
                        r->method = Method::CONNECT;
                    }

                    break;

                case 8:
                    if (ngx_str8cmp(m, 'P', 'R', 'O', 'P', 'F', 'I', 'N', 'D'))
                    {
                        r->method = Method::PROPFIND;
                    }

                    break;

                case 9:
                    if (ngx_str9cmp(m, 'P', 'R', 'O', 'P', 'P', 'A', 'T', 'C', 'H'))
                    {
                        r->method = Method::PROPPATCH;
                    }

                    break;
                }

                state = RequestState::sw_spaces_before_uri;
                break;
            }

            if ((ch < 'A' || ch > 'Z') && ch != '_' && ch != '-')
            {
                return ERROR;
            }

            break;

        /* space* before URI */
        case RequestState::sw_spaces_before_uri:

            if (ch == '/')
            {
                r->uri_start = p;
                state = RequestState::sw_after_slash_in_uri;
                break;
            }

            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'z')
            {
                r->schema_start = p;
                state = RequestState::sw_schema;
                break;
            }

            switch (ch)
            {
            case ' ':
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::sw_schema:

            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'z')
            {
                break;
            }

            if ((ch >= '0' && ch <= '9') || ch == '+' || ch == '-' || ch == '.')
            {
                break;
            }

            switch (ch)
            {
            case ':':
                r->schema_end = p;
                state = RequestState::sw_schema_slash;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::sw_schema_slash:
            switch (ch)
            {
            case '/':
                state = RequestState::sw_schema_slash_slash;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::sw_schema_slash_slash:
            switch (ch)
            {
            case '/':
                state = RequestState::sw_host_start;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::sw_host_start:

            r->host_start = p;

            if (ch == '[')
            {
                state = RequestState::sw_host_ip_literal;
                break;
            }

            state = RequestState::sw_host;

            /* fall through */

        case RequestState::sw_host:

            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'z')
            {
                break;
            }

            if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-')
            {
                break;
            }

            /* fall through */

        case RequestState::sw_host_end:

            r->host_end = p;

            switch (ch)
            {
            case ':':
                state = RequestState::sw_port;
                break;
            case '/':
                r->uri_start = p;
                state = RequestState::sw_after_slash_in_uri;
                break;
            case '?':
                r->uri_start = p;
                r->args_start = p + 1;
                r->empty_path_in_uri = 1;
                state = RequestState::sw_uri;
                break;
            case ' ':
                /*
                 * use single "/" from request line to preserve pointers,
                 * if request line will be copied to large client buffer
                 */
                r->uri_start = r->schema_end + 1;
                r->uri_end = r->schema_end + 2;
                state = RequestState::sw_http_09;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::sw_host_ip_literal:

            if (ch >= '0' && ch <= '9')
            {
                break;
            }

            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'z')
            {
                break;
            }

            switch (ch)
            {
            case ':':
                break;
            case ']':
                state = RequestState::sw_host_end;
                break;
            case '-':
            case '.':
            case '_':
            case '~':
                /* unreserved */
                break;
            case '!':
            case '$':
            case '&':
            case '\'':
            case '(':
            case ')':
            case '*':
            case '+':
            case ',':
            case ';':
            case '=':
                /* sub-delims */
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::sw_port:
            if (ch >= '0' && ch <= '9')
            {
                break;
            }

            switch (ch)
            {
            case '/':
                r->port_end = p;
                r->uri_start = p;
                state = RequestState::sw_after_slash_in_uri;
                break;
            case '?':
                r->port_end = p;
                r->uri_start = p;
                r->args_start = p + 1;
                r->empty_path_in_uri = 1;
                state = RequestState::sw_uri;
                break;
            case ' ':
                r->port_end = p;
                /*
                 * use single "/" from request line to preserve pointers,
                 * if request line will be copied to large client buffer
                 */
                r->uri_start = r->schema_end + 1;
                r->uri_end = r->schema_end + 2;
                state = RequestState::sw_http_09;
                break;
            default:
                return ERROR;
            }
            break;

        /* check "/.", "//", "%", and "\" (Win32) in URI */
        case RequestState::sw_after_slash_in_uri:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                state = RequestState::sw_check_uri;
                break;
            }

            switch (ch)
            {
            case ' ':
                r->uri_end = p;
                state = RequestState::sw_http_09;
                break;
            case CR:
                r->uri_end = p;
                r->http_minor = 9;
                state = RequestState::sw_almost_done;
                break;
            case LF:
                r->uri_end = p;
                r->http_minor = 9;
                goto done;
            case '.':
                r->complex_uri = 1;
                state = RequestState::sw_uri;
                break;
            case '%':
                r->quoted_uri = 1;
                state = RequestState::sw_uri;
                break;
            case '/':
                r->complex_uri = 1;
                state = RequestState::sw_uri;
                break;
            case '?':
                r->args_start = p + 1;
                state = RequestState::sw_uri;
                break;
            case '#':
                r->complex_uri = 1;
                state = RequestState::sw_uri;
                break;
            case '+':
                r->plus_in_uri = 1;
                break;
            default:
                if (ch < 0x20 || ch == 0x7f)
                {
                    return ERROR;
                }
                state = RequestState::sw_check_uri;
                break;
            }
            break;

        /* check "/", "%" and "\" (Win32) in URI */
        case RequestState::sw_check_uri:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                break;
            }

            switch (ch)
            {
            case '/':
                r->uri_ext = NULL;
                state = RequestState::sw_after_slash_in_uri;
                break;
            case '.':
                r->uri_ext = p + 1;
                break;
            case ' ':
                r->uri_end = p;
                state = RequestState::sw_http_09;
                break;
            case CR:
                r->uri_end = p;
                r->http_minor = 9;
                state = RequestState::sw_almost_done;
                break;
            case LF:
                r->uri_end = p;
                r->http_minor = 9;
                goto done;
            case '%':
                r->quoted_uri = 1;
                state = RequestState::sw_uri;
                break;
            case '?':
                r->args_start = p + 1;
                state = RequestState::sw_uri;
                break;
            case '#':
                r->complex_uri = 1;
                state = RequestState::sw_uri;
                break;
            case '+':
                r->plus_in_uri = 1;
                break;
            default:
                if (ch < 0x20 || ch == 0x7f)
                {
                    return ERROR;
                }
                break;
            }
            break;

        /* URI */
        case RequestState::sw_uri:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                break;
            }

            switch (ch)
            {
            case ' ':
                r->uri_end = p;
                state = RequestState::sw_http_09;
                break;
            case CR:
                r->uri_end = p;
                r->http_minor = 9;
                state = RequestState::sw_almost_done;
                break;
            case LF:
                r->uri_end = p;
                r->http_minor = 9;
                goto done;
            case '#':
                r->complex_uri = 1;
                break;
            default:
                if (ch < 0x20 || ch == 0x7f)
                {
                    return ERROR;
                }
                break;
            }
            break;

        /* space+ after URI */
        case RequestState::sw_http_09:
            switch (ch)
            {
            case ' ':
                break;
            case CR:
                r->http_minor = 9;
                state = RequestState::sw_almost_done;
                break;
            case LF:
                r->http_minor = 9;
                goto done;
            case 'H':
                r->http_protocol.data = p;
                state = RequestState::sw_http_H;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::sw_http_H:
            switch (ch)
            {
            case 'T':
                state = RequestState::sw_http_HT;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::sw_http_HT:
            switch (ch)
            {
            case 'T':
                state = RequestState::sw_http_HTT;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::sw_http_HTT:
            switch (ch)
            {
            case 'P':
                state = RequestState::sw_http_HTTP;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::sw_http_HTTP:
            switch (ch)
            {
            case '/':
                state = RequestState::sw_first_major_digit;
                break;
            default:
                return ERROR;
            }
            break;

        /* first digit of major HTTP version */
        case RequestState::sw_first_major_digit:
            if (ch < '1' || ch > '9')
            {
                return ERROR;
            }

            r->http_major = ch - '0';

            if (r->http_major > 1)
            {
                return ERROR;
            }

            state = RequestState::sw_major_digit;
            break;

        /* major HTTP version or dot */
        case RequestState::sw_major_digit:
            if (ch == '.')
            {
                state = RequestState::sw_first_minor_digit;
                break;
            }

            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            r->http_major = r->http_major * 10 + (ch - '0');

            if (r->http_major > 1)
            {
                return ERROR;
            }

            break;

        /* first digit of minor HTTP version */
        case RequestState::sw_first_minor_digit:
            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            r->http_minor = ch - '0';
            state = RequestState::sw_minor_digit;
            break;

        /* minor HTTP version or end of request line */
        case RequestState::sw_minor_digit:
            if (ch == CR)
            {
                state = RequestState::sw_almost_done;
                break;
            }

            if (ch == LF)
            {
                goto done;
            }

            if (ch == ' ')
            {
                state = RequestState::sw_spaces_after_digit;
                break;
            }

            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            if (r->http_minor > 99)
            {
                return ERROR;
            }

            r->http_minor = r->http_minor * 10 + (ch - '0');
            break;

        case RequestState::sw_spaces_after_digit:
            switch (ch)
            {
            case ' ':
                break;
            case CR:
                state = RequestState::sw_almost_done;
                break;
            case LF:
                goto done;
            default:
                return ERROR;
            }
            break;

        /* end of request line */
        case RequestState::sw_almost_done:
            r->request_end = p - 1;
            switch (ch)
            {
            case LF:
                goto done;
            default:
                return ERROR;
            }
        }
    }

    buffer.now->pos = p - buffer.now->start;
    // r->c->readBuffer_.retrieveUntil((const char *)(p));
    return AGAIN;

done:

    buffer.now->pos = p + 1 - buffer.now->start;
    // r->c->readBuffer_.retrieveUntil((const char *)(p + 1));

    if (r->request_end == NULL)
    {
        r->request_end = p;
    }

    r->http_version = r->http_major * 1000 + r->http_minor;
    r->requestState = RequestState::sw_start;

    if (r->http_version == 9 && r->method != Method::GET)
    {
        return ERROR;
    }

    return OK;
}

int parseComplexUri(std::shared_ptr<Request> r, int merge_slashes)
{
    u_char c, ch, decoded, *p, *u;
    enum
    {
        sw_usual = 0,
        sw_slash,
        sw_dot,
        sw_dot_dot,
        sw_quoted,
        sw_quoted_second
    } state, quoted_state;

    state = sw_usual;
    p = r->uri_start;
    u = r->uri.data;
    r->uri_ext = NULL;
    r->args_start = NULL;

    if (r->empty_path_in_uri)
    {
        *u++ = '/';
    }

    ch = *p++;

    while (p <= r->uri_end)
    {

        /*
         * we use "ch = *p++" inside the cycle, but this operation is safe,
         * because after the URI there is always at least one character:
         * the line feed
         */

        switch (state)
        {

        case sw_usual:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                *u++ = ch;
                ch = *p++;
                break;
            }

            switch (ch)
            {
            case '/':
                r->uri_ext = NULL;
                state = sw_slash;
                *u++ = ch;
                break;
            case '%':
                quoted_state = state;
                state = sw_quoted;
                break;
            case '?':
                r->args_start = p;
                goto args;
            case '#':
                goto done;
            case '.':
                r->uri_ext = u + 1;
                *u++ = ch;
                break;
            case '+':
                r->plus_in_uri = 1;
                /* fall through */
            default:
                *u++ = ch;
                break;
            }

            ch = *p++;
            break;

        case sw_slash:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                state = sw_usual;
                *u++ = ch;
                ch = *p++;
                break;
            }

            switch (ch)
            {
            case '/':
                if (!merge_slashes)
                {
                    *u++ = ch;
                }
                break;
            case '.':
                state = sw_dot;
                *u++ = ch;
                break;
            case '%':
                quoted_state = state;
                state = sw_quoted;
                break;
            case '?':
                r->args_start = p;
                goto args;
            case '#':
                goto done;
            case '+':
                r->plus_in_uri = 1;
                /* fall through */
            default:
                state = sw_usual;
                *u++ = ch;
                break;
            }

            ch = *p++;
            break;

        case sw_dot:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                state = sw_usual;
                *u++ = ch;
                ch = *p++;
                break;
            }

            switch (ch)
            {
            case '/':
                state = sw_slash;
                u--;
                break;
            case '.':
                state = sw_dot_dot;
                *u++ = ch;
                break;
            case '%':
                quoted_state = state;
                state = sw_quoted;
                break;
            case '?':
                u--;
                r->args_start = p;
                goto args;
            case '#':
                u--;
                goto done;
            case '+':
                r->plus_in_uri = 1;
                /* fall through */
            default:
                state = sw_usual;
                *u++ = ch;
                break;
            }

            ch = *p++;
            break;

        case sw_dot_dot:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                state = sw_usual;
                *u++ = ch;
                ch = *p++;
                break;
            }

            switch (ch)
            {
            case '/':
            case '?':
            case '#':
                u -= 4;
                for (;;)
                {
                    if (u < r->uri.data)
                    {
                        return ERROR;
                    }
                    if (*u == '/')
                    {
                        u++;
                        break;
                    }
                    u--;
                }
                if (ch == '?')
                {
                    r->args_start = p;
                    goto args;
                }
                if (ch == '#')
                {
                    goto done;
                }
                state = sw_slash;
                break;
            case '%':
                quoted_state = state;
                state = sw_quoted;
                break;
            case '+':
                r->plus_in_uri = 1;
                /* fall through */
            default:
                state = sw_usual;
                *u++ = ch;
                break;
            }

            ch = *p++;
            break;

        case sw_quoted:
            r->quoted_uri = 1;

            if (ch >= '0' && ch <= '9')
            {
                decoded = (u_char)(ch - '0');
                state = sw_quoted_second;
                ch = *p++;
                break;
            }

            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'f')
            {
                decoded = (u_char)(c - 'a' + 10);
                state = sw_quoted_second;
                ch = *p++;
                break;
            }

            return ERROR;

        case sw_quoted_second:
            if (ch >= '0' && ch <= '9')
            {
                ch = (u_char)((decoded << 4) + (ch - '0'));

                if (ch == '%' || ch == '#')
                {
                    state = sw_usual;
                    *u++ = ch;
                    ch = *p++;
                    break;
                }
                else if (ch == '\0')
                {
                    return ERROR;
                }

                state = quoted_state;
                break;
            }

            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'f')
            {
                ch = (u_char)((decoded << 4) + (c - 'a') + 10);

                if (ch == '?')
                {
                    state = sw_usual;
                    *u++ = ch;
                    ch = *p++;
                    break;
                }
                else if (ch == '+')
                {
                    r->plus_in_uri = 1;
                }

                state = quoted_state;
                break;
            }

            return ERROR;
        }
    }

    if (state == sw_quoted || state == sw_quoted_second)
    {
        return ERROR;
    }

    if (state == sw_dot)
    {
        u--;
    }
    else if (state == sw_dot_dot)
    {
        u -= 4;

        for (;;)
        {
            if (u < r->uri.data)
            {
                return ERROR;
            }

            if (*u == '/')
            {
                u++;
                break;
            }

            u--;
        }
    }

done:

    r->uri.len = u - r->uri.data;

    if (r->uri_ext)
    {
        r->exten.len = u - r->uri_ext;
        r->exten.data = r->uri_ext;
    }

    r->uri_ext = NULL;

    return OK;

args:

    while (p < r->uri_end)
    {
        if (*p++ != '#')
        {
            continue;
        }

        r->args.len = p - 1 - r->args_start;
        r->args.data = r->args_start;
        r->args_start = NULL;

        break;
    }

    r->uri.len = u - r->uri.data;

    if (r->uri_ext)
    {
        r->exten.len = u - r->uri_ext;
        r->exten.data = r->uri_ext;
    }

    r->uri_ext = NULL;

    return OK;
}

int parseHeaderLine(std::shared_ptr<Request> r, int allow_underscores)
{
    u_char c, ch, *p;
    // unsigned long hash;
    unsigned long i;

    /* the last '\0' is not needed because string is zero terminated */

    static u_char lowcase[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                              "\0\0\0\0\0\0\0\0\0\0\0\0\0-\0\0"
                              "0123456789\0\0\0\0\0\0"
                              "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
                              "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
                              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                              "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

    HeaderState &state = r->headerState;
    // hash = r->header_hash;
    i = r->lowcase_index;
    auto &buffer = r->c->readBuffer_;

    for (p = buffer.now->start + buffer.now->pos; p < buffer.now->start + buffer.now->len; p++)
    {
        ch = *p;

        switch (state)
        {

        /* first char */
        case HeaderState::sw_start:
            r->header_name_start = p;
            r->invalid_header = 0;

            switch (ch)
            {
            case CR:
                r->header_end = p;
                state = HeaderState::sw_header_almost_done;
                break;
            case LF:
                r->header_end = p;
                goto header_done;
            default:
                state = HeaderState::sw_name;

                c = lowcase[ch];

                if (c)
                {
                    // hash = ngx_hash(0, c);
                    r->lowcase_header[0] = c;
                    i = 1;
                    break;
                }

                if (ch == '_')
                {
                    if (allow_underscores)
                    {
                        // hash = ngx_hash(0, ch);
                        r->lowcase_header[0] = ch;
                        i = 1;
                    }
                    else
                    {
                        // hash = 0;
                        i = 0;
                        r->invalid_header = 1;
                    }

                    break;
                }

                if (ch <= 0x20 || ch == 0x7f || ch == ':')
                {
                    r->header_end = p;
                    return ERROR;
                }

                // hash = 0;
                i = 0;
                r->invalid_header = 1;

                break;
            }
            break;

        /* header name */
        case HeaderState::sw_name:
            c = lowcase[ch];

            if (c)
            {
                // hash = ngx_hash(hash, c);
                r->lowcase_header[i++] = c;
                i &= (32 - 1);
                break;
            }

            if (ch == '_')
            {
                if (allow_underscores)
                {
                    // hash = ngx_hash(hash, ch);
                    r->lowcase_header[i++] = ch;
                    i &= (32 - 1);
                }
                else
                {
                    r->invalid_header = 1;
                }

                break;
            }

            if (ch == ':')
            {
                r->header_name_end = p;
                state = HeaderState::sw_space_before_value;
                break;
            }

            if (ch == CR)
            {
                r->header_name_end = p;
                r->header_start = p;
                r->header_end = p;
                state = HeaderState::sw_almost_done;
                break;
            }

            if (ch == LF)
            {
                r->header_name_end = p;
                r->header_start = p;
                r->header_end = p;
                goto done;
            }

            // /* IIS may send the duplicate "HTTP/1.1 ..." lines */
            // if (ch == '/' && r->upstream && p - r->header_name_start == 4 &&
            //     ngx_strncmp(r->header_name_start, "HTTP", 4) == 0)
            // {
            //     state = sw_ignore_line;
            //     break;
            // }

            if (ch <= 0x20 || ch == 0x7f)
            {
                r->header_end = p;
                return ERROR;
            }

            r->invalid_header = 1;

            break;

        /* space* before header value */
        case HeaderState::sw_space_before_value:
            switch (ch)
            {
            case ' ':
                break;
            case CR:
                r->header_start = p;
                r->header_end = p;
                state = HeaderState::sw_almost_done;
                break;
            case LF:
                r->header_start = p;
                r->header_end = p;
                goto done;
            case '\0':
                r->header_end = p;
                return ERROR;
            default:
                r->header_start = p;
                state = HeaderState::sw_value;
                break;
            }
            break;

        /* header value */
        case HeaderState::sw_value:
            switch (ch)
            {
            case ' ':
                r->header_end = p;
                state = HeaderState::sw_space_after_value;
                break;
            case CR:
                r->header_end = p;
                state = HeaderState::sw_almost_done;
                break;
            case LF:
                r->header_end = p;
                goto done;
            case '\0':
                r->header_end = p;
                return ERROR;
            }
            break;

        /* space* before end of header line */
        case HeaderState::sw_space_after_value:
            switch (ch)
            {
            case ' ':
                break;
            case CR:
                state = HeaderState::sw_almost_done;
                break;
            case LF:
                goto done;
            case '\0':
                r->header_end = p;
                return ERROR;
            default:
                state = HeaderState::sw_value;
                break;
            }
            break;

        /* ignore header line */
        case HeaderState::sw_ignore_line:
            switch (ch)
            {
            case LF:
                state = HeaderState::sw_start;
                break;
            default:
                break;
            }
            break;

        /* end of header line */
        case HeaderState::sw_almost_done:
            switch (ch)
            {
            case LF:
                goto done;
            case CR:
                break;
            default:
                return ERROR;
            }
            break;

        /* end of header */
        case HeaderState::sw_header_almost_done:
            switch (ch)
            {
            case LF:
                goto header_done;
            default:
                return ERROR;
            }
        }
    }

    buffer.now->pos = p - buffer.now->start;
    // r->c->readBuffer_.retrieveUntil((char *)p);
    // r->header_hash = hash;
    r->lowcase_index = i;

    return AGAIN;

done:

    buffer.now->pos = p + 1 - buffer.now->start;
    // r->c->readBuffer_.retrieveUntil((char *)(p + 1));
    r->headerState = HeaderState::sw_start;
    // r->header_hash = hash;
    r->lowcase_index = i;

    return OK;

header_done:

    buffer.now->pos = p + 1 - buffer.now->start;
    // r->c->readBuffer_.retrieveUntil((char *)(p + 1));
    r->headerState = HeaderState::sw_start;

    return DONE;
}

#define MAX_OFF_T_VALUE 9223372036854775807LL

int parseChunked(std::shared_ptr<Request> r)
{
    u_char *pos, ch, c;
    int rc;
    int once = 0;
    int left;

    ChunkedInfo *ctx = &r->request_body.chunkedInfo;
    auto &buffer = r->c->readBuffer_;

    auto &state = ctx->state;

    if (state == ChunkedState::sw_chunk_data && ctx->size == 0)
    {
        state = ChunkedState::sw_after_data;
    }

    rc = AGAIN;

    // printf("%s\n", buffer.now->toString().c_str());
    // u_char *tmp = buffer.now->start + buffer.now->pos + ctx->data_offset;
    // if (tmp < buffer.now->start + buffer.now->len)
    // {
    //     printf("value:%d\n",*tmp);
    // }

    for (pos = buffer.now->start + buffer.now->pos + ctx->data_offset; pos < buffer.now->start + buffer.now->len; pos++)
    {
        // printf("%d %d ", ctx->data_offset, *pos);
        once = 1;
        ch = *pos;

        switch (state)
        {

        case ChunkedState::sw_chunk_start:
            if (ch >= '0' && ch <= '9')
            {
                state = ChunkedState::sw_chunk_size;
                ctx->size = ch - '0';
                break;
            }

            c = (u_char)(ch | 0x20);

            if (c >= 'a' && c <= 'f')
            {
                state = ChunkedState::sw_chunk_size;
                ctx->size = c - 'a' + 10;
                break;
            }
            goto invalid;

        case ChunkedState::sw_chunk_size:
            if (ctx->size > MAX_OFF_T_VALUE / 16)
            {
                goto invalid;
            }

            if (ch >= '0' && ch <= '9')
            {
                ctx->size = ctx->size * 16 + (ch - '0');
                break;
            }

            c = (u_char)(ch | 0x20);

            if (c >= 'a' && c <= 'f')
            {
                ctx->size = ctx->size * 16 + (c - 'a' + 10);
                break;
            }

            if (ctx->size == 0)
            {

                switch (ch)
                {
                case CR:
                    state = ChunkedState::sw_last_chunk_extension_almost_done;
                    break;
                case LF:
                    state = ChunkedState::sw_trailer;
                    break;
                case ';':
                case ' ':
                case '\t':
                    state = ChunkedState::sw_last_chunk_extension;
                    break;
                default:
                    goto invalid;
                }

                break;
            }

            switch (ch)
            {
            case CR:
                state = ChunkedState::sw_chunk_extension_almost_done;
                break;
            case LF:
                state = ChunkedState::sw_chunk_data;
                break;
            case ';':
            case ' ':
            case '\t':
                state = ChunkedState::sw_chunk_extension;
                break;
            default:
                goto invalid;
            }

            break;

        case ChunkedState::sw_chunk_extension:
            switch (ch)
            {
            case CR:
                state = ChunkedState::sw_chunk_extension_almost_done;
                break;
            case LF:
                state = ChunkedState::sw_chunk_data;
            }
            break;

        case ChunkedState::sw_chunk_extension_almost_done:
            if (ch == LF)
            {
                state = ChunkedState::sw_chunk_data;
                break;
            }
            goto invalid;

        case ChunkedState::sw_chunk_data:
            rc = OK;
            goto data;

        case ChunkedState::sw_after_data:
            switch (ch)
            {
            case CR:
                state = ChunkedState::sw_after_data_almost_done;
                break;
            case LF:
                state = ChunkedState::sw_chunk_start;
                break;
            default:
                goto invalid;
            }
            break;

        case ChunkedState::sw_after_data_almost_done:
            if (ch == LF)
            {
                state = ChunkedState::sw_chunk_start;
                break;
            }
            goto invalid;

        case ChunkedState::sw_last_chunk_extension:
            switch (ch)
            {
            case CR:
                state = ChunkedState::sw_last_chunk_extension_almost_done;
                break;
            case LF:
                state = ChunkedState::sw_trailer;
            }
            break;

        case ChunkedState::sw_last_chunk_extension_almost_done:
            if (ch == LF)
            {
                state = ChunkedState::sw_trailer;
                break;
            }
            goto invalid;

        case ChunkedState::sw_trailer:
            switch (ch)
            {
            case CR:
                state = ChunkedState::sw_trailer_almost_done;
                break;
            case LF:
                goto done;
            default:
                state = ChunkedState::sw_trailer_header;
            }
            break;

        case ChunkedState::sw_trailer_almost_done:
            if (ch == LF)
            {
                goto done;
            }
            goto invalid;

        case ChunkedState::sw_trailer_header:
            switch (ch)
            {
            case CR:
                state = ChunkedState::sw_trailer_header_almost_done;
                break;
            case LF:
                state = ChunkedState::sw_trailer;
            }
            break;

        case ChunkedState::sw_trailer_header_almost_done:
            if (ch == LF)
            {
                state = ChunkedState::sw_trailer;
                break;
            }
            goto invalid;
        }
    }

data:

    if (rc==OK)
    {
        ctx->data_offset = ctx->size;
    }

    if (rc==OK) // right after chunked size, we need to add the chunk size too!
    {
        left = pos - buffer.now->start -
               buffer.now->pos; // only add "SIZE\r\n", *pos supposed to be the first byte of data
        r->request_body.lbody.emplace_back(buffer.now->start + buffer.now->pos, left);
        buffer.now->pos += left;
    }
    else // add chunked data
    {
        if (once) // means this buffer contains all of this chunk, and the \r\n after
        {
            left = pos - buffer.now->start - buffer.now->pos;
            ctx->data_offset = 0;
        }
        else // chunk is larger than this buffer, just add them all
        {
            left = buffer.now->len - buffer.now->pos;
            ctx->data_offset -= left;
        }
        r->request_body.lbody.emplace_back(buffer.now->start + buffer.now->pos, left);
        buffer.now->pos += left;
    }

    if (ctx->size > MAX_OFF_T_VALUE - 5)
    {
        goto invalid;
    }

    ctx->size = 0;

    return rc;

done:

    // *pos is the last \n

    left = pos + 1 - buffer.now->start - buffer.now->pos;
    r->request_body.lbody.emplace_back(buffer.now->start + buffer.now->pos, left);

    // prepare for the next time
    ctx->state = ChunkedState::sw_chunk_start;
    ctx->size = 0;
    ctx->data_offset = 0;

    buffer.retrieve(left);

    return DONE;

invalid:
    return ERROR;
}

int parseStatusLine(std::shared_ptr<Request> r, Status *status)
{
    u_char ch;
    u_char *p;

    auto &state = r->responseState;
    auto &buffer = r->c->readBuffer_;

    for (p = buffer.now->start + buffer.now->pos; p < buffer.now->start + buffer.now->len; p++)
    {
        ch = *p;

        switch (state)
        {

        /* "HTTP/" */
        case ResponseState::sw_start:
            switch (ch)
            {
            case 'H':
                state = ResponseState::sw_H;
                break;
            default:
                return ERROR;
            }
            break;

        case ResponseState::sw_H:
            switch (ch)
            {
            case 'T':
                state = ResponseState::sw_HT;
                break;
            default:
                return ERROR;
            }
            break;

        case ResponseState::sw_HT:
            switch (ch)
            {
            case 'T':
                state = ResponseState::sw_HTT;
                break;
            default:
                return ERROR;
            }
            break;

        case ResponseState::sw_HTT:
            switch (ch)
            {
            case 'P':
                state = ResponseState::sw_HTTP;
                break;
            default:
                return ERROR;
            }
            break;

        case ResponseState::sw_HTTP:
            switch (ch)
            {
            case '/':
                state = ResponseState::sw_first_major_digit;
                break;
            default:
                return ERROR;
            }
            break;

        /* the first digit of major HTTP version */
        case ResponseState::sw_first_major_digit:
            if (ch < '1' || ch > '9')
            {
                return ERROR;
            }

            r->http_major = ch - '0';
            state = ResponseState::sw_major_digit;
            break;

        /* the major HTTP version or dot */
        case ResponseState::sw_major_digit:
            if (ch == '.')
            {
                state = ResponseState::sw_first_minor_digit;
                break;
            }

            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            if (r->http_major > 99)
            {
                return ERROR;
            }

            r->http_major = r->http_major * 10 + (ch - '0');
            break;

        /* the first digit of minor HTTP version */
        case ResponseState::sw_first_minor_digit:
            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            r->http_minor = ch - '0';
            state = ResponseState::sw_minor_digit;
            break;

        /* the minor HTTP version or the end of the request line */
        case ResponseState::sw_minor_digit:
            if (ch == ' ')
            {
                state = ResponseState::sw_status;
                break;
            }

            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            if (r->http_minor > 99)
            {
                return ERROR;
            }

            r->http_minor = r->http_minor * 10 + (ch - '0');
            break;

        /* HTTP status code */
        case ResponseState::sw_status:
            if (ch == ' ')
            {
                break;
            }

            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            // status->code = status->code * 10 + (ch - '0');

            if (++status->count == 3)
            {
                state = ResponseState::sw_space_after_status;
                status->start = p - 2;
            }

            break;

        /* space or end of line */
        case ResponseState::sw_space_after_status:
            switch (ch)
            {
            case ' ':
                state = ResponseState::sw_status_text;
                break;
            case '.': /* IIS may send 403.1, 403.2, etc */
                state = ResponseState::sw_status_text;
                break;
            case CR:
                state = ResponseState::sw_almost_done;
                break;
            case LF:
                goto done;
            default:
                return ERROR;
            }
            break;

        /* any text until end of line */
        case ResponseState::sw_status_text:
            switch (ch)
            {
            case CR:
                state = ResponseState::sw_almost_done;

                break;
            case LF:
                goto done;
            }
            break;

        /* end of status line */
        case ResponseState::sw_almost_done:
            status->end = p - 1;
            switch (ch)
            {
            case LF:
                goto done;
            default:
                return ERROR;
            }
        }
    }

    buffer.now->pos = p - buffer.now->start;
    // r->c->readBuffer_.retrieveUntil((char *)(p));

    return AGAIN;

done:

    buffer.now->pos = p + 1 - buffer.now->start;
    // r->c->readBuffer_.retrieveUntil((char *)(p + 1));

    if (status->end == NULL)
    {
        status->end = p;
    }

    status->http_version = r->http_major * 1000 + r->http_minor;
    state = ResponseState::sw_start;

    return OK;
}