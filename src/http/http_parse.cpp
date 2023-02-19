#include "http_parse.h"
#include "http.h"

extern Cycle *cyclePtr;

int parseRequestLine(Request *r)
{
    LOG_INFO << "parse request line";
    u_char c, ch, *p, *m;

    auto &state = r->state;

    for (p = (u_char *)r->c->readBuffer_.peek(); p < (u_char *)r->c->readBuffer_.beginWrite(); p++)
    {
        ch = *p;

        switch (state)
        {

        /* HTTP methods: GET, HEAD, POST */
        case State::sw_start:
            r->request_start = p;

            if (ch == CR || ch == LF)
            {
                break;
            }

            if ((ch < 'A' || ch > 'Z') && ch != '_' && ch != '-')
            {
                return ERROR;
            }

            state = State::sw_method;
            break;

        case State::sw_method:
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

                state = State::sw_spaces_before_uri;
                break;
            }

            if ((ch < 'A' || ch > 'Z') && ch != '_' && ch != '-')
            {
                return ERROR;
            }

            break;

        /* space* before URI */
        case State::sw_spaces_before_uri:

            if (ch == '/')
            {
                r->uri_start = p;
                state = State::sw_after_slash_in_uri;
                break;
            }

            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'z')
            {
                r->schema_start = p;
                state = State::sw_schema;
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

        case State::sw_schema:

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
                state = State::sw_schema_slash;
                break;
            default:
                return ERROR;
            }
            break;

        case State::sw_schema_slash:
            switch (ch)
            {
            case '/':
                state = State::sw_schema_slash_slash;
                break;
            default:
                return ERROR;
            }
            break;

        case State::sw_schema_slash_slash:
            switch (ch)
            {
            case '/':
                state = State::sw_host_start;
                break;
            default:
                return ERROR;
            }
            break;

        case State::sw_host_start:

            r->host_start = p;

            if (ch == '[')
            {
                state = State::sw_host_ip_literal;
                break;
            }

            state = State::sw_host;

            /* fall through */

        case State::sw_host:

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

        case State::sw_host_end:

            r->host_end = p;

            switch (ch)
            {
            case ':':
                state = State::sw_port;
                break;
            case '/':
                r->uri_start = p;
                state = State::sw_after_slash_in_uri;
                break;
            case '?':
                r->uri_start = p;
                r->args_start = p + 1;
                r->empty_path_in_uri = 1;
                state = State::sw_uri;
                break;
            case ' ':
                /*
                 * use single "/" from request line to preserve pointers,
                 * if request line will be copied to large client buffer
                 */
                r->uri_start = r->schema_end + 1;
                r->uri_end = r->schema_end + 2;
                state = State::sw_http_09;
                break;
            default:
                return ERROR;
            }
            break;

        case State::sw_host_ip_literal:

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
                state = State::sw_host_end;
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

        case State::sw_port:
            if (ch >= '0' && ch <= '9')
            {
                break;
            }

            switch (ch)
            {
            case '/':
                r->port_end = p;
                r->uri_start = p;
                state = State::sw_after_slash_in_uri;
                break;
            case '?':
                r->port_end = p;
                r->uri_start = p;
                r->args_start = p + 1;
                r->empty_path_in_uri = 1;
                state = State::sw_uri;
                break;
            case ' ':
                r->port_end = p;
                /*
                 * use single "/" from request line to preserve pointers,
                 * if request line will be copied to large client buffer
                 */
                r->uri_start = r->schema_end + 1;
                r->uri_end = r->schema_end + 2;
                state = State::sw_http_09;
                break;
            default:
                return ERROR;
            }
            break;

        /* check "/.", "//", "%", and "\" (Win32) in URI */
        case State::sw_after_slash_in_uri:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                state = State::sw_check_uri;
                break;
            }

            switch (ch)
            {
            case ' ':
                r->uri_end = p;
                state = State::sw_http_09;
                break;
            case CR:
                r->uri_end = p;
                r->http_minor = 9;
                state = State::sw_almost_done;
                break;
            case LF:
                r->uri_end = p;
                r->http_minor = 9;
                goto done;
            case '.':
                r->complex_uri = 1;
                state = State::sw_uri;
                break;
            case '%':
                r->quoted_uri = 1;
                state = State::sw_uri;
                break;
            case '/':
                r->complex_uri = 1;
                state = State::sw_uri;
                break;
            case '?':
                r->args_start = p + 1;
                state = State::sw_uri;
                break;
            case '#':
                r->complex_uri = 1;
                state = State::sw_uri;
                break;
            case '+':
                r->plus_in_uri = 1;
                break;
            default:
                if (ch < 0x20 || ch == 0x7f)
                {
                    return ERROR;
                }
                state = State::sw_check_uri;
                break;
            }
            break;

        /* check "/", "%" and "\" (Win32) in URI */
        case State::sw_check_uri:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                break;
            }

            switch (ch)
            {
            case '/':
                r->uri_ext = NULL;
                state = State::sw_after_slash_in_uri;
                break;
            case '.':
                r->uri_ext = p + 1;
                break;
            case ' ':
                r->uri_end = p;
                state = State::sw_http_09;
                break;
            case CR:
                r->uri_end = p;
                r->http_minor = 9;
                state = State::sw_almost_done;
                break;
            case LF:
                r->uri_end = p;
                r->http_minor = 9;
                goto done;
            case '%':
                r->quoted_uri = 1;
                state = State::sw_uri;
                break;
            case '?':
                r->args_start = p + 1;
                state = State::sw_uri;
                break;
            case '#':
                r->complex_uri = 1;
                state = State::sw_uri;
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
        case State::sw_uri:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                break;
            }

            switch (ch)
            {
            case ' ':
                r->uri_end = p;
                state = State::sw_http_09;
                break;
            case CR:
                r->uri_end = p;
                r->http_minor = 9;
                state = State::sw_almost_done;
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
        case State::sw_http_09:
            switch (ch)
            {
            case ' ':
                break;
            case CR:
                r->http_minor = 9;
                state = State::sw_almost_done;
                break;
            case LF:
                r->http_minor = 9;
                goto done;
            case 'H':
                r->http_protocol.data = p;
                state = State::sw_http_H;
                break;
            default:
                return ERROR;
            }
            break;

        case State::sw_http_H:
            switch (ch)
            {
            case 'T':
                state = State::sw_http_HT;
                break;
            default:
                return ERROR;
            }
            break;

        case State::sw_http_HT:
            switch (ch)
            {
            case 'T':
                state = State::sw_http_HTT;
                break;
            default:
                return ERROR;
            }
            break;

        case State::sw_http_HTT:
            switch (ch)
            {
            case 'P':
                state = State::sw_http_HTTP;
                break;
            default:
                return ERROR;
            }
            break;

        case State::sw_http_HTTP:
            switch (ch)
            {
            case '/':
                state = State::sw_first_major_digit;
                break;
            default:
                return ERROR;
            }
            break;

        /* first digit of major HTTP version */
        case State::sw_first_major_digit:
            if (ch < '1' || ch > '9')
            {
                return ERROR;
            }

            r->http_major = ch - '0';

            if (r->http_major > 1)
            {
                return ERROR;
            }

            state = State::sw_major_digit;
            break;

        /* major HTTP version or dot */
        case State::sw_major_digit:
            if (ch == '.')
            {
                state = State::sw_first_minor_digit;
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
        case State::sw_first_minor_digit:
            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            r->http_minor = ch - '0';
            state = State::sw_minor_digit;
            break;

        /* minor HTTP version or end of request line */
        case State::sw_minor_digit:
            if (ch == CR)
            {
                state = State::sw_almost_done;
                break;
            }

            if (ch == LF)
            {
                goto done;
            }

            if (ch == ' ')
            {
                state = State::sw_spaces_after_digit;
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

        case State::sw_spaces_after_digit:
            switch (ch)
            {
            case ' ':
                break;
            case CR:
                state = State::sw_almost_done;
                break;
            case LF:
                goto done;
            default:
                return ERROR;
            }
            break;

        /* end of request line */
        case State::sw_almost_done:
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

    r->c->readBuffer_.retrieveUntil((const char *)(p));
    return AGAIN;

done:

    r->c->readBuffer_.retrieveUntil((const char *)(p + 1));

    if (r->request_end == NULL)
    {
        r->request_end = p;
    }

    r->http_version = r->http_major * 1000 + r->http_minor;
    r->state = State::sw_start;

    if (r->http_version == 9 && r->method != Method::GET)
    {
        return ERROR;
    }

    return OK;
}