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

// GET /example/path HTTP/1.1\r\n
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
        case RequestState::START:
            r->requestStart = p;

            if (ch == CR || ch == LF)
            {
                break;
            }

            if ((ch < 'A' || ch > 'Z') && ch != '_' && ch != '-')
            {
                return ERROR;
            }

            state = RequestState::METHOD;
            break;

        case RequestState::METHOD:
            // continue until we meet the space after method
            if (ch == ' ')
            {
                r->methodEnd = p - 1;
                m = r->requestStart;

                // switch between method length, like GET, POST
                switch (p - m)
                {

                case 3:
                    if (str4cmp(m, 'G', 'E', 'T', ' '))
                    {
                        r->method = Method::GET;
                        break;
                    }

                    if (str4cmp(m, 'P', 'U', 'T', ' '))
                    {
                        r->method = Method::PUT;
                        break;
                    }

                    break;

                case 4:
                    if (m[1] == 'O')
                    {

                        if (str4cmp(m, 'P', 'O', 'S', 'T'))
                        {
                            r->method = Method::POST;
                            break;
                        }

                        if (str4cmp(m, 'C', 'O', 'P', 'Y'))
                        {
                            r->method = Method::COPY;
                            break;
                        }

                        if (str4cmp(m, 'M', 'O', 'V', 'E'))
                        {
                            r->method = Method::MOVE;
                            break;
                        }

                        if (str4cmp(m, 'L', 'O', 'C', 'K'))
                        {
                            r->method = Method::LOCK;
                            break;
                        }
                    }
                    else
                    {

                        if (str4cmp(m, 'H', 'E', 'A', 'D'))
                        {
                            r->method = Method::HEAD;
                            break;
                        }
                    }

                    break;

                case 5:
                    if (str5cmp(m, 'M', 'K', 'C', 'O', 'L'))
                    {
                        r->method = Method::MKCOL;
                        break;
                    }

                    if (str5cmp(m, 'P', 'A', 'T', 'C', 'H'))
                    {
                        r->method = Method::PATCH;
                        break;
                    }

                    if (str5cmp(m, 'T', 'R', 'A', 'C', 'E'))
                    {
                        r->method = Method::TRACE;
                        break;
                    }

                    break;

                case 6:
                    if (str6cmp(m, 'D', 'E', 'L', 'E', 'T', 'E'))
                    {
                        r->method = Method::DELETE;
                        break;
                    }

                    if (str6cmp(m, 'U', 'N', 'L', 'O', 'C', 'K'))
                    {
                        r->method = Method::UNLOCK;
                        break;
                    }

                    break;

                case 7:
                    if (str8cmp(m, 'O', 'P', 'T', 'I', 'O', 'N', 'S', ' '))
                    {
                        r->method = Method::OPTIONS;
                    }

                    if (str8cmp(m, 'C', 'O', 'N', 'N', 'E', 'C', 'T', ' '))
                    {
                        r->method = Method::CONNECT;
                    }

                    break;

                case 8:
                    if (str8cmp(m, 'P', 'R', 'O', 'P', 'F', 'I', 'N', 'D'))
                    {
                        r->method = Method::PROPFIND;
                    }

                    break;

                case 9:
                    if (str9cmp(m, 'P', 'R', 'O', 'P', 'P', 'A', 'T', 'C', 'H'))
                    {
                        r->method = Method::PROPPATCH;
                    }

                    break;
                }

                state = RequestState::SPACE_BEFORE_URI;
                break;
            }

            if ((ch < 'A' || ch > 'Z') && ch != '_' && ch != '-')
            {
                return ERROR;
            }

            break;

        // GET /example/path HTTP/1.1\r\n
        case RequestState::SPACE_BEFORE_URI:

            if (ch == '/')
            {
                r->uriStart = p;
                state = RequestState::AFTER_SLASH_URI;
                break;
            }

            // turn ch to lowercase
            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'z')
            {
                r->schemaStart = p;
                state = RequestState::SCHEMA;
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

        case RequestState::SCHEMA:

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
                r->schemaEnd = p;
                state = RequestState::SCHEMA_SLASH0;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::SCHEMA_SLASH0:
            switch (ch)
            {
            case '/':
                state = RequestState::SCHEMA_SLASH1;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::SCHEMA_SLASH1:
            switch (ch)
            {
            case '/':
                state = RequestState::HOST_START;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::HOST_START:

            r->hostStart = p;

            if (ch == '[')
            {
                state = RequestState::HOST_IP;
                break;
            }

            state = RequestState::HOST;

            // fall through

        case RequestState::HOST:

            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'z')
            {
                break;
            }

            if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-')
            {
                break;
            }

            // fall through

        case RequestState::HOST_END:

            r->hostEnd = p;

            switch (ch)
            {
            case ':':
                state = RequestState::PORT;
                break;
            case '/':
                r->uriStart = p;
                state = RequestState::AFTER_SLASH_URI;
                break;
            case '?':
                r->uriStart = p;
                r->argsStart = p + 1;
                r->emptyPathInUri = 1;
                state = RequestState::URI;
                break;
            case ' ':
                /*
                 * use single "/" from request line to preserve pointers,
                 * if request line will be copied to large client buffer
                 */
                r->uriStart = r->schemaEnd + 1;
                r->uriEnd = r->schemaEnd + 2;
                state = RequestState::HTTP_09;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::HOST_IP:

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
                state = RequestState::HOST_END;
                break;
            case '-':
            case '.':
            case '_':
            case '~':
                // unreserved
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
                // sub-delims
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::PORT:
            if (ch >= '0' && ch <= '9')
            {
                break;
            }

            switch (ch)
            {
            case '/':
                r->portEnd = p;
                r->uriStart = p;
                state = RequestState::AFTER_SLASH_URI;
                break;
            case '?':
                r->portEnd = p;
                r->uriStart = p;
                r->argsStart = p + 1;
                r->emptyPathInUri = 1;
                state = RequestState::URI;
                break;
            case ' ':
                r->portEnd = p;
                /*
                 * use single "/" from request line to preserve pointers,
                 * if request line will be copied to large client buffer
                 */
                r->uriStart = r->schemaEnd + 1;
                r->uriEnd = r->schemaEnd + 2;
                state = RequestState::HTTP_09;
                break;
            default:
                return ERROR;
            }
            break;

        // check "/.", "//", "%" in URI
        case RequestState::AFTER_SLASH_URI:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                state = RequestState::CHECK_URI;
                break;
            }

            switch (ch)
            {
            case ' ':
                r->uriEnd = p;
                state = RequestState::HTTP_09;
                break;
            case CR:
                r->uriEnd = p;
                r->httpMinor = 9;
                state = RequestState::REQUEST_DONE;
                break;
            case LF:
                r->uriEnd = p;
                r->httpMinor = 9;
                goto done;
            case '.':
                r->complexUri = 1;
                state = RequestState::URI;
                break;
            case '%':
                r->quotedUri = 1;
                state = RequestState::URI;
                break;
            case '/':
                r->complexUri = 1;
                state = RequestState::URI;
                break;
            case '?':
                r->argsStart = p + 1;
                state = RequestState::URI;
                break;
            case '#':
                r->complexUri = 1;
                state = RequestState::URI;
                break;
            case '+':
                r->plusInUri = 1;
                break;
            default:
                if (ch < 0x20 || ch == 0x7f)
                {
                    return ERROR;
                }
                state = RequestState::CHECK_URI;
                break;
            }
            break;

        // check "/", "%" in URI
        case RequestState::CHECK_URI:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                break;
            }

            switch (ch)
            {
            case '/':
                r->uriExt = NULL;
                state = RequestState::AFTER_SLASH_URI;
                break;
            case '.':
                r->uriExt = p + 1;
                break;
            case ' ':
                r->uriEnd = p;
                state = RequestState::HTTP_09;
                break;
            case CR:
                r->uriEnd = p;
                r->httpMinor = 9;
                state = RequestState::REQUEST_DONE;
                break;
            case LF:
                r->uriEnd = p;
                r->httpMinor = 9;
                goto done;
            case '%':
                r->quotedUri = 1;
                state = RequestState::URI;
                break;
            case '?':
                r->argsStart = p + 1;
                state = RequestState::URI;
                break;
            case '#':
                r->complexUri = 1;
                state = RequestState::URI;
                break;
            case '+':
                r->plusInUri = 1;
                break;
            default:
                if (ch < 0x20 || ch == 0x7f)
                {
                    return ERROR;
                }
                break;
            }
            break;

        case RequestState::URI:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                break;
            }

            switch (ch)
            {
            case ' ':
                r->uriEnd = p;
                state = RequestState::HTTP_09;
                break;
            case CR:
                r->uriEnd = p;
                r->httpMinor = 9;
                state = RequestState::REQUEST_DONE;
                break;
            case LF:
                r->uriEnd = p;
                r->httpMinor = 9;
                goto done;
            case '#':
                r->complexUri = 1;
                break;
            default:
                if (ch < 0x20 || ch == 0x7f)
                {
                    return ERROR;
                }
                break;
            }
            break;

        case RequestState::HTTP_09:
            switch (ch)
            {
            case ' ':
                break;
            case CR:
                r->httpMinor = 9;
                state = RequestState::REQUEST_DONE;
                break;
            case LF:
                r->httpMinor = 9;
                goto done;
            case 'H':
                r->protocol.data = p;
                state = RequestState::HTTP_H;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::HTTP_H:
            switch (ch)
            {
            case 'T':
                state = RequestState::HTTP_HT;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::HTTP_HT:
            switch (ch)
            {
            case 'T':
                state = RequestState::HTTP_HTT;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::HTTP_HTT:
            switch (ch)
            {
            case 'P':
                state = RequestState::HTTP_HTTP;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::HTTP_HTTP:
            switch (ch)
            {
            case '/':
                state = RequestState::FIRST_MAJOR_DIGIT;
                break;
            default:
                return ERROR;
            }
            break;

        // first digit of major HTTP version
        case RequestState::FIRST_MAJOR_DIGIT:
            if (ch < '1' || ch > '9')
            {
                return ERROR;
            }

            r->httpMajor = ch - '0';

            if (r->httpMajor > 1)
            {
                return ERROR;
            }

            state = RequestState::MAJOR_DIGIT;
            break;

        // major HTTP version or dot
        case RequestState::MAJOR_DIGIT:
            if (ch == '.')
            {
                state = RequestState::FIRST_MINOR_DIGIT;
                break;
            }

            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            r->httpMajor = r->httpMajor * 10 + (ch - '0');

            if (r->httpMajor > 1)
            {
                return ERROR;
            }

            break;

        // first digit of minor HTTP version
        case RequestState::FIRST_MINOR_DIGIT:
            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            r->httpMinor = ch - '0';
            state = RequestState::MINOR_DIGIT;
            break;

        // minor HTTP version or end of request line
        case RequestState::MINOR_DIGIT:
            if (ch == CR)
            {
                state = RequestState::REQUEST_DONE;
                break;
            }

            if (ch == LF)
            {
                goto done;
            }

            if (ch == ' ')
            {
                state = RequestState::SPACES_AFTER_DIGIT;
                break;
            }

            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            if (r->httpMinor > 99)
            {
                return ERROR;
            }

            r->httpMinor = r->httpMinor * 10 + (ch - '0');
            break;

        case RequestState::SPACES_AFTER_DIGIT:
            switch (ch)
            {
            case ' ':
                break;
            case CR:
                state = RequestState::REQUEST_DONE;
                break;
            case LF:
                goto done;
            default:
                return ERROR;
            }
            break;

        // end of request line
        case RequestState::REQUEST_DONE:
            r->requestEnd = p - 1;
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

    return AGAIN;

done:

    buffer.now->pos = p + 1 - buffer.now->start;

    if (r->requestEnd == NULL)
    {
        r->requestEnd = p;
    }

    r->httpVersion = r->httpMajor * 1000 + r->httpMinor;
    r->requestState = RequestState::START;

    if (r->httpVersion == 9 && r->method != Method::GET)
    {
        return ERROR;
    }

    return OK;
}

int parseComplexUri(std::shared_ptr<Request> r, int mergeSlashes)
{
    u_char c, ch, decoded, *p, *u;
    enum
    {
        USUAL = 0,
        SLASH,
        DOT,
        DOT_DOT,
        QUOTED,
        QUOTED_SECOND
    } state, quoted_state;

    state = USUAL;
    p = r->uriStart;
    u = r->uri.data;
    r->uriExt = NULL;
    r->argsStart = NULL;

    if (r->emptyPathInUri)
    {
        *u++ = '/';
    }

    ch = *p++;

    while (p <= r->uriEnd)
    {
        switch (state)
        {

        case USUAL:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                *u++ = ch;
                ch = *p++;
                break;
            }

            switch (ch)
            {
            case '/':
                r->uriExt = NULL;
                state = SLASH;
                *u++ = ch;
                break;
            case '%':
                quoted_state = state;
                state = QUOTED;
                break;
            case '?':
                r->argsStart = p;
                goto args;
            case '#':
                goto done;
            case '.':
                r->uriExt = u + 1;
                *u++ = ch;
                break;
            case '+':
                r->plusInUri = 1;
                // fall through
            default:
                *u++ = ch;
                break;
            }

            ch = *p++;
            break;

        case SLASH:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                state = USUAL;
                *u++ = ch;
                ch = *p++;
                break;
            }

            switch (ch)
            {
            case '/':
                if (!mergeSlashes)
                {
                    *u++ = ch;
                }
                break;
            case '.':
                state = DOT;
                *u++ = ch;
                break;
            case '%':
                quoted_state = state;
                state = QUOTED;
                break;
            case '?':
                r->argsStart = p;
                goto args;
            case '#':
                goto done;
            case '+':
                r->plusInUri = 1;
                // fall through
            default:
                state = USUAL;
                *u++ = ch;
                break;
            }

            ch = *p++;
            break;

        case DOT:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                state = USUAL;
                *u++ = ch;
                ch = *p++;
                break;
            }

            switch (ch)
            {
            case '/':
                state = SLASH;
                u--;
                break;
            case '.':
                state = DOT_DOT;
                *u++ = ch;
                break;
            case '%':
                quoted_state = state;
                state = QUOTED;
                break;
            case '?':
                u--;
                r->argsStart = p;
                goto args;
            case '#':
                u--;
                goto done;
            case '+':
                r->plusInUri = 1;
                // fall through
            default:
                state = USUAL;
                *u++ = ch;
                break;
            }

            ch = *p++;
            break;

        case DOT_DOT:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                state = USUAL;
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
                    r->argsStart = p;
                    goto args;
                }
                if (ch == '#')
                {
                    goto done;
                }
                state = SLASH;
                break;
            case '%':
                quoted_state = state;
                state = QUOTED;
                break;
            case '+':
                r->plusInUri = 1;
                // fall through
            default:
                state = USUAL;
                *u++ = ch;
                break;
            }

            ch = *p++;
            break;

        case QUOTED:
            r->quotedUri = 1;

            if (ch >= '0' && ch <= '9')
            {
                decoded = (u_char)(ch - '0');
                state = QUOTED_SECOND;
                ch = *p++;
                break;
            }

            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'f')
            {
                decoded = (u_char)(c - 'a' + 10);
                state = QUOTED_SECOND;
                ch = *p++;
                break;
            }

            return ERROR;

        case QUOTED_SECOND:
            if (ch >= '0' && ch <= '9')
            {
                ch = (u_char)((decoded << 4) + (ch - '0'));

                if (ch == '%' || ch == '#')
                {
                    state = USUAL;
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
                    state = USUAL;
                    *u++ = ch;
                    ch = *p++;
                    break;
                }
                else if (ch == '+')
                {
                    r->plusInUri = 1;
                }

                state = quoted_state;
                break;
            }

            return ERROR;
        }
    }

    if (state == QUOTED || state == QUOTED_SECOND)
    {
        return ERROR;
    }

    if (state == DOT)
    {
        u--;
    }
    else if (state == DOT_DOT)
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

    if (r->uriExt)
    {
        r->exten.len = u - r->uriExt;
        r->exten.data = r->uriExt;
    }

    r->uriExt = NULL;

    return OK;

args:

    while (p < r->uriEnd)
    {
        if (*p++ != '#')
        {
            continue;
        }

        r->args.len = p - 1 - r->argsStart;
        r->args.data = r->argsStart;
        r->argsStart = NULL;

        break;
    }

    r->uri.len = u - r->uri.data;

    if (r->uriExt)
    {
        r->exten.len = u - r->uriExt;
        r->exten.data = r->uriExt;
    }

    r->uriExt = NULL;

    return OK;
}

// Host: www.example.com\r\n
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n
// Accept-Language: en-US,en;q=0.5\r\n
// Connection: keep-alive\r\n
// \r\n
int parseHeaderLine(std::shared_ptr<Request> r, bool allowUnderscores)
{
    u_char c, ch, *p;

    // the last '\0' is not needed because string is zero terminated
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
    auto &buffer = r->c->readBuffer_;

    for (p = buffer.now->start + buffer.now->pos; p < buffer.now->start + buffer.now->len; p++)
    {
        ch = *p;

        switch (state)
        {
        case HeaderState::START:
            r->headerNameStart = p;
            r->invalidHeader = 0;

            // switch between normal character and line-ender
            // may end with \n or \r\n
            switch (ch)
            {
            case CR:
                r->headerValueEnd = p;
                state = HeaderState::HEADERS_DONE;
                break;
            case LF:
                r->headerValueEnd = p;
                goto header_done;
            default:
                state = HeaderState::NAME;

                c = lowcase[ch];

                // continue if current char is valid
                if (c)
                {
                    break;
                }

                if (ch == '_')
                {
                    if (allowUnderscores)
                    {
                    }
                    else
                    {
                        r->invalidHeader = 1;
                    }
                    break;
                }

                // invalid char, check ascii table to learn more
                // unlike invalid header, we need to return ERROR right away
                if (ch <= 0x20 || ch == 0x7f || ch == ':')
                {
                    r->headerValueEnd = p;
                    return ERROR;
                }

                r->invalidHeader = 1;

                break;
            }
            break;

        case HeaderState::NAME:
            c = lowcase[ch];

            if (c)
            {
                break;
            }

            if (ch == '_')
            {
                if (allowUnderscores)
                {
                }
                else
                {
                    r->invalidHeader = 1;
                }
                break;
            }

            if (ch == ':')
            {
                r->headerNameEnd = p;
                state = HeaderState::SPACE0;
                break;
            }

            // we can just set invalidHeader when encountering CR or LF
            if (ch == CR)
            {
                r->headerNameEnd = p;
                r->headerValueStart = p;
                r->headerValueEnd = p;
                state = HeaderState::LINE_DONE;
                break;
            }

            if (ch == LF)
            {
                r->headerNameEnd = p;
                r->headerValueStart = p;
                r->headerValueEnd = p;
                goto done;
            }

            if (ch <= 0x20 || ch == 0x7f)
            {
                r->headerValueEnd = p;
                return ERROR;
            }

            r->invalidHeader = 1;

            break;

        // space before header value
        case HeaderState::SPACE0:
            switch (ch)
            {
            case ' ':
                break;
            case CR:
                r->headerValueStart = p;
                r->headerValueEnd = p;
                state = HeaderState::LINE_DONE;
                break;
            case LF:
                r->headerValueStart = p;
                r->headerValueEnd = p;
                goto done;
            case '\0':
                r->headerValueEnd = p;
                return ERROR;
            default:
                r->headerValueStart = p;
                state = HeaderState::VALUE;
                break;
            }
            break;

        case HeaderState::VALUE:
            switch (ch)
            {
            case ' ':
                r->headerValueEnd = p;
                state = HeaderState::SPACE1;
                break;
            case CR:
                r->headerValueEnd = p;
                state = HeaderState::LINE_DONE;
                break;
            case LF:
                r->headerValueEnd = p;
                goto done;
            case '\0':
                r->headerValueEnd = p;
                return ERROR;
            }
            break;

        // space before end of header line
        case HeaderState::SPACE1:
            switch (ch)
            {
            case ' ':
                break;
            case CR:
                state = HeaderState::LINE_DONE;
                break;
            case LF:
                goto done;
            case '\0':
                r->headerValueEnd = p;
                return ERROR;
            default:
                state = HeaderState::VALUE;
                break;
            }
            break;

        case HeaderState::IGNORE:
            switch (ch)
            {
            case LF:
                state = HeaderState::START;
                break;
            default:
                break;
            }
            break;

        case HeaderState::LINE_DONE:
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

        case HeaderState::HEADERS_DONE:
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

    return AGAIN;

done:

    buffer.now->pos = p + 1 - buffer.now->start;
    r->headerState = HeaderState::START;

    return OK;

header_done:

    buffer.now->pos = p + 1 - buffer.now->start;
    r->headerState = HeaderState::START;

    return DONE;
}

#define MAX_OFF_T_VALUE 9223372036854775807LL

int parseChunked(std::shared_ptr<Request> r)
{
    u_char *pos, ch, c;
    int rc;
    int once = 0;
    int left;

    ChunkedInfo *ctx = &r->requestBody.chunkedInfo;
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

    for (pos = buffer.now->start + buffer.now->pos + ctx->dataOffset; pos < buffer.now->start + buffer.now->len; pos++)
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

    if (rc == OK)
    {
        ctx->dataOffset = ctx->size;
    }

    if (rc == OK) // right after chunked size, we need to add the chunk size too!
    {
        left = pos - buffer.now->start -
               buffer.now->pos; // only add "SIZE\r\n", *pos supposed to be the first byte of data
        r->requestBody.listBody.emplace_back(buffer.now->start + buffer.now->pos, left);
        buffer.now->pos += left;
    }
    else // add chunked data
    {
        if (once) // means this buffer contains all of this chunk, and the \r\n after
        {
            left = pos - buffer.now->start - buffer.now->pos;
            ctx->dataOffset = 0;
        }
        else // chunk is larger than this buffer, just add them all
        {
            left = buffer.now->len - buffer.now->pos;
            ctx->dataOffset -= left;
        }
        r->requestBody.listBody.emplace_back(buffer.now->start + buffer.now->pos, left);
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
    r->requestBody.listBody.emplace_back(buffer.now->start + buffer.now->pos, left);

    // prepare for the next time
    ctx->state = ChunkedState::sw_chunk_start;
    ctx->size = 0;
    ctx->dataOffset = 0;

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

            r->httpMajor = ch - '0';
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

            if (r->httpMajor > 99)
            {
                return ERROR;
            }

            r->httpMajor = r->httpMajor * 10 + (ch - '0');
            break;

        /* the first digit of minor HTTP version */
        case ResponseState::sw_first_minor_digit:
            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            r->httpMinor = ch - '0';
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

            if (r->httpMinor > 99)
            {
                return ERROR;
            }

            r->httpMinor = r->httpMinor * 10 + (ch - '0');
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

    status->httpVersion = r->httpMajor * 1000 + r->httpMinor;
    state = ResponseState::sw_start;

    return OK;
}