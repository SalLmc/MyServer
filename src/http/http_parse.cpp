#include "../headers.h"

#include "../core/core.h"
#include "http.h"
#include "http_parse.h"

// usage: usual[ch >> 5] & (1U << (ch & 0x1f))
// if the expression above is true then the char is in this table
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

    auto &state = r->requestState_;
    auto &buffer = r->c_->readBuffer_;

    for (p = buffer.pivot_->start_ + buffer.pivot_->pos_; p < buffer.pivot_->start_ + buffer.pivot_->len_; p++)
    {
        ch = *p;

        switch (state)
        {
        case RequestState::START:
            r->requestStart_ = p;

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
                r->methodEnd_ = p - 1;
                m = r->requestStart_;

                // switch between method length, like GET, POST
                switch (p - m)
                {

                case 3:
                    if (str4cmp(m, 'G', 'E', 'T', ' '))
                    {
                        r->method_ = Method::GET;
                        break;
                    }

                    if (str4cmp(m, 'P', 'U', 'T', ' '))
                    {
                        r->method_ = Method::PUT;
                        break;
                    }

                    break;

                case 4:
                    if (m[1] == 'O')
                    {

                        if (str4cmp(m, 'P', 'O', 'S', 'T'))
                        {
                            r->method_ = Method::POST;
                            break;
                        }

                        if (str4cmp(m, 'C', 'O', 'P', 'Y'))
                        {
                            r->method_ = Method::COPY;
                            break;
                        }

                        if (str4cmp(m, 'M', 'O', 'V', 'E'))
                        {
                            r->method_ = Method::MOVE;
                            break;
                        }

                        if (str4cmp(m, 'L', 'O', 'C', 'K'))
                        {
                            r->method_ = Method::LOCK;
                            break;
                        }
                    }
                    else
                    {

                        if (str4cmp(m, 'H', 'E', 'A', 'D'))
                        {
                            r->method_ = Method::HEAD;
                            break;
                        }
                    }

                    break;

                case 5:
                    if (str5cmp(m, 'M', 'K', 'C', 'O', 'L'))
                    {
                        r->method_ = Method::MKCOL;
                        break;
                    }

                    if (str5cmp(m, 'P', 'A', 'T', 'C', 'H'))
                    {
                        r->method_ = Method::PATCH;
                        break;
                    }

                    if (str5cmp(m, 'T', 'R', 'A', 'C', 'E'))
                    {
                        r->method_ = Method::TRACE;
                        break;
                    }

                    break;

                case 6:
                    if (str6cmp(m, 'D', 'E', 'L', 'E', 'T', 'E'))
                    {
                        r->method_ = Method::DELETE;
                        break;
                    }

                    if (str6cmp(m, 'U', 'N', 'L', 'O', 'C', 'K'))
                    {
                        r->method_ = Method::UNLOCK;
                        break;
                    }

                    break;

                case 7:
                    if (str8cmp(m, 'O', 'P', 'T', 'I', 'O', 'N', 'S', ' '))
                    {
                        r->method_ = Method::OPTIONS;
                    }

                    if (str8cmp(m, 'C', 'O', 'N', 'N', 'E', 'C', 'T', ' '))
                    {
                        r->method_ = Method::CONNECT;
                    }

                    break;

                case 8:
                    if (str8cmp(m, 'P', 'R', 'O', 'P', 'F', 'I', 'N', 'D'))
                    {
                        r->method_ = Method::PROPFIND;
                    }

                    break;

                case 9:
                    if (str9cmp(m, 'P', 'R', 'O', 'P', 'P', 'A', 'T', 'C', 'H'))
                    {
                        r->method_ = Method::PROPPATCH;
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
                r->uriStart_ = p;
                state = RequestState::AFTER_SLASH_URI;
                break;
            }

            // turn ch to lowercase
            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'z')
            {
                r->schemaStart_ = p;
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
                r->schemaEnd_ = p;
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

            r->hostStart_ = p;

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

            r->hostEnd_ = p;

            switch (ch)
            {
            case ':':
                state = RequestState::PORT;
                break;
            case '/':
                r->uriStart_ = p;
                state = RequestState::AFTER_SLASH_URI;
                break;
            case '?':
                r->uriStart_ = p;
                r->argsStart_ = p + 1;
                r->emptyPathInUri_ = 1;
                state = RequestState::URI;
                break;
            case ' ':
                /*
                 * use single "/" from request line to preserve pointers,
                 * if request line will be copied to large client buffer
                 */
                r->uriStart_ = r->schemaEnd_ + 1;
                r->uriEnd_ = r->schemaEnd_ + 2;
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
                r->portEnd_ = p;
                r->uriStart_ = p;
                state = RequestState::AFTER_SLASH_URI;
                break;
            case '?':
                r->portEnd_ = p;
                r->uriStart_ = p;
                r->argsStart_ = p + 1;
                r->emptyPathInUri_ = 1;
                state = RequestState::URI;
                break;
            case ' ':
                r->portEnd_ = p;
                /*
                 * use single "/" from request line to preserve pointers,
                 * if request line will be copied to large client buffer
                 */
                r->uriStart_ = r->schemaEnd_ + 1;
                r->uriEnd_ = r->schemaEnd_ + 2;
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
                r->uriEnd_ = p;
                state = RequestState::HTTP_09;
                break;
            case CR:
                r->uriEnd_ = p;
                r->httpMinor_ = 9;
                state = RequestState::REQUEST_DONE;
                break;
            case LF:
                r->uriEnd_ = p;
                r->httpMinor_ = 9;
                goto done;
            case '.':
                r->complexUri_ = 1;
                state = RequestState::URI;
                break;
            case '%':
                r->quotedUri_ = 1;
                state = RequestState::URI;
                break;
            case '/':
                r->complexUri_ = 1;
                state = RequestState::URI;
                break;
            case '?':
                r->argsStart_ = p + 1;
                state = RequestState::URI;
                break;
            case '#':
                r->complexUri_ = 1;
                state = RequestState::URI;
                break;
            case '+':
                r->plusInUri_ = 1;
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
                r->uriExt_ = NULL;
                state = RequestState::AFTER_SLASH_URI;
                break;
            case '.':
                r->uriExt_ = p + 1;
                break;
            case ' ':
                r->uriEnd_ = p;
                state = RequestState::HTTP_09;
                break;
            case CR:
                r->uriEnd_ = p;
                r->httpMinor_ = 9;
                state = RequestState::REQUEST_DONE;
                break;
            case LF:
                r->uriEnd_ = p;
                r->httpMinor_ = 9;
                goto done;
            case '%':
                r->quotedUri_ = 1;
                state = RequestState::URI;
                break;
            case '?':
                r->argsStart_ = p + 1;
                state = RequestState::URI;
                break;
            case '#':
                r->complexUri_ = 1;
                state = RequestState::URI;
                break;
            case '+':
                r->plusInUri_ = 1;
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
                r->uriEnd_ = p;
                state = RequestState::HTTP_09;
                break;
            case CR:
                r->uriEnd_ = p;
                r->httpMinor_ = 9;
                state = RequestState::REQUEST_DONE;
                break;
            case LF:
                r->uriEnd_ = p;
                r->httpMinor_ = 9;
                goto done;
            case '#':
                r->complexUri_ = 1;
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
                r->httpMinor_ = 9;
                state = RequestState::REQUEST_DONE;
                break;
            case LF:
                r->httpMinor_ = 9;
                goto done;
            case 'H':
                r->protocol_.data_ = p;
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

            r->httpMajor_ = ch - '0';

            if (r->httpMajor_ > 1)
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

            r->httpMajor_ = r->httpMajor_ * 10 + (ch - '0');

            if (r->httpMajor_ > 1)
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

            r->httpMinor_ = ch - '0';
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

            if (r->httpMinor_ > 99)
            {
                return ERROR;
            }

            r->httpMinor_ = r->httpMinor_ * 10 + (ch - '0');
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
            r->requestEnd_ = p - 1;
            switch (ch)
            {
            case LF:
                goto done;
            default:
                return ERROR;
            }
        }
    }

    buffer.pivot_->pos_ = p - buffer.pivot_->start_;

    return AGAIN;

done:

    buffer.pivot_->pos_ = p + 1 - buffer.pivot_->start_;

    if (r->requestEnd_ == NULL)
    {
        r->requestEnd_ = p;
    }

    r->httpVersion_ = r->httpMajor_ * 1000 + r->httpMinor_;
    r->requestState_ = RequestState::START;

    if (r->httpVersion_ == 9 && r->method_ != Method::GET)
    {
        return ERROR;
    }

    return OK;
}

// r->complexUri || r->quotedUri || r->emptyPathInUri
// already malloc a new space for uri.data
// 1. decode chars begin with '%' in QUOTED
// 2. extract exten in state like DOT
// 3. handle ./ and ../
int parseComplexUri(std::shared_ptr<Request> r, int mergeSlashes)
{
    u_char c;
    u_char ch;
    u_char decoded;
    u_char *old, *now;

    enum
    {
        USUAL = 0,
        SLASH,
        DOT,
        DOT_DOT,
        QUOTED,
        QUOTED_SECOND
    } state, saveState;

    state = USUAL;

    old = r->uriStart_;
    now = r->uri_.data_;
    r->uriExt_ = NULL;
    r->argsStart_ = NULL;

    if (r->emptyPathInUri_)
    {
        // *now++ <=> *now='/'; now++
        *now++ = '/';
    }

    ch = *old++;

    while (old <= r->uriEnd_)
    {
        switch (state)
        {

        case USUAL:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                *now++ = ch;
                ch = *old++;
                break;
            }

            switch (ch)
            {
            case '/':
                r->uriExt_ = NULL;
                state = SLASH;
                *now++ = ch;
                break;
            case '%':
                saveState = state;
                state = QUOTED;
                break;
            case '?':
                r->argsStart_ = old;
                goto args;
            case '#':
                goto done;
            case '.':
                r->uriExt_ = now + 1;
                *now++ = ch;
                break;
            case '+':
                r->plusInUri_ = 1;
                // fall through
            default:
                *now++ = ch;
                break;
            }

            ch = *old++;
            break;

        case SLASH:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                state = USUAL;
                *now++ = ch;
                ch = *old++;
                break;
            }

            switch (ch)
            {
            case '/':
                if (!mergeSlashes)
                {
                    *now++ = ch;
                }
                break;
            case '.':
                state = DOT;
                *now++ = ch;
                break;
            case '%':
                saveState = state;
                state = QUOTED;
                break;
            case '?':
                r->argsStart_ = old;
                goto args;
            case '#':
                goto done;
            case '+':
                r->plusInUri_ = 1;
                // fall through
            default:
                state = USUAL;
                *now++ = ch;
                break;
            }

            ch = *old++;
            break;

        case DOT:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                state = USUAL;
                *now++ = ch;
                ch = *old++;
                break;
            }

            switch (ch)
            {
            case '/':
                state = SLASH;
                now--;
                break;
            case '.':
                state = DOT_DOT;
                *now++ = ch;
                break;
            case '%':
                saveState = state;
                state = QUOTED;
                break;
            case '?':
                now--;
                r->argsStart_ = old;
                goto args;
            case '#':
                now--;
                goto done;
            case '+':
                r->plusInUri_ = 1;
                // fall through
            default:
                state = USUAL;
                *now++ = ch;
                break;
            }

            ch = *old++;
            break;

        case DOT_DOT:

            if (usual[ch >> 5] & (1U << (ch & 0x1f)))
            {
                state = USUAL;
                *now++ = ch;
                ch = *old++;
                break;
            }

            switch (ch)
            {
            case '/':
            case '?':
            case '#':
                now -= 4;
                for (;;)
                {
                    if (now < r->uri_.data_)
                    {
                        return ERROR;
                    }
                    if (*now == '/')
                    {
                        now++;
                        break;
                    }
                    now--;
                }
                if (ch == '?')
                {
                    r->argsStart_ = old;
                    goto args;
                }
                if (ch == '#')
                {
                    goto done;
                }
                state = SLASH;
                break;
            case '%':
                saveState = state;
                state = QUOTED;
                break;
            case '+':
                r->plusInUri_ = 1;
                // fall through
            default:
                state = USUAL;
                *now++ = ch;
                break;
            }

            ch = *old++;
            break;

        case QUOTED:
            r->quotedUri_ = 1;

            if (ch >= '0' && ch <= '9')
            {
                decoded = (u_char)(ch - '0');
                state = QUOTED_SECOND;
                ch = *old++;
                break;
            }

            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'f')
            {
                decoded = (u_char)(c - 'a' + 10);
                state = QUOTED_SECOND;
                ch = *old++;
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
                    *now++ = ch;
                    ch = *old++;
                    break;
                }
                else if (ch == '\0')
                {
                    return ERROR;
                }

                state = saveState;
                break;
            }

            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'f')
            {
                ch = (u_char)((decoded << 4) + (c - 'a') + 10);

                if (ch == '?')
                {
                    state = USUAL;
                    *now++ = ch;
                    ch = *old++;
                    break;
                }
                else if (ch == '+')
                {
                    r->plusInUri_ = 1;
                }

                state = saveState;
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
        now--;
    }
    else if (state == DOT_DOT)
    {
        now -= 4;

        for (;;)
        {
            if (now < r->uri_.data_)
            {
                return ERROR;
            }

            if (*now == '/')
            {
                now++;
                break;
            }

            now--;
        }
    }

done:

    r->uri_.len_ = now - r->uri_.data_;

    if (r->uriExt_)
    {
        r->exten_.len_ = now - r->uriExt_;
        r->exten_.data_ = r->uriExt_;
    }

    r->uriExt_ = NULL;

    return OK;

args:

    while (old < r->uriEnd_)
    {
        if (*old++ != '#')
        {
            continue;
        }

        r->args_.len_ = old - 1 - r->argsStart_;
        r->args_.data_ = r->argsStart_;
        r->argsStart_ = NULL;

        break;
    }

    r->uri_.len_ = now - r->uri_.data_;

    if (r->uriExt_)
    {
        r->exten_.len_ = now - r->uriExt_;
        r->exten_.data_ = r->uriExt_;
    }

    r->uriExt_ = NULL;

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

    HeaderState &state = r->headerState_;
    auto &buffer = r->c_->readBuffer_;

    for (p = buffer.pivot_->start_ + buffer.pivot_->pos_; p < buffer.pivot_->start_ + buffer.pivot_->len_; p++)
    {
        ch = *p;

        switch (state)
        {
        case HeaderState::START:
            r->headerNameStart_ = p;
            r->invalidHeader_ = 0;

            // switch between normal character and line-ender
            // may end with \n or \r\n
            switch (ch)
            {
            case CR:
                r->headerValueEnd_ = p;
                state = HeaderState::HEADERS_DONE;
                break;
            case LF:
                r->headerValueEnd_ = p;
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
                        r->invalidHeader_ = 1;
                    }
                    break;
                }

                // invalid char, check ascii table to learn more
                // unlike invalid header, we need to return ERROR right away
                if (ch <= 0x20 || ch == 0x7f || ch == ':')
                {
                    r->headerValueEnd_ = p;
                    return ERROR;
                }

                r->invalidHeader_ = 1;

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
                    r->invalidHeader_ = 1;
                }
                break;
            }

            if (ch == ':')
            {
                r->headerNameEnd_ = p;
                state = HeaderState::SPACE0;
                break;
            }

            // we can just set invalidHeader when encountering CR or LF
            if (ch == CR)
            {
                r->headerNameEnd_ = p;
                r->headerValueStart_ = p;
                r->headerValueEnd_ = p;
                state = HeaderState::LINE_DONE;
                break;
            }

            if (ch == LF)
            {
                r->headerNameEnd_ = p;
                r->headerValueStart_ = p;
                r->headerValueEnd_ = p;
                goto done;
            }

            if (ch <= 0x20 || ch == 0x7f)
            {
                r->headerValueEnd_ = p;
                return ERROR;
            }

            r->invalidHeader_ = 1;

            break;

        // space before header value
        case HeaderState::SPACE0:
            switch (ch)
            {
            case ' ':
                break;
            case CR:
                r->headerValueStart_ = p;
                r->headerValueEnd_ = p;
                state = HeaderState::LINE_DONE;
                break;
            case LF:
                r->headerValueStart_ = p;
                r->headerValueEnd_ = p;
                goto done;
            case '\0':
                r->headerValueEnd_ = p;
                return ERROR;
            default:
                r->headerValueStart_ = p;
                state = HeaderState::VALUE;
                break;
            }
            break;

        case HeaderState::VALUE:
            switch (ch)
            {
            case ' ':
                r->headerValueEnd_ = p;
                state = HeaderState::SPACE1;
                break;
            case CR:
                r->headerValueEnd_ = p;
                state = HeaderState::LINE_DONE;
                break;
            case LF:
                r->headerValueEnd_ = p;
                goto done;
            case '\0':
                r->headerValueEnd_ = p;
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
                r->headerValueEnd_ = p;
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

    buffer.pivot_->pos_ = p - buffer.pivot_->start_;

    return AGAIN;

done:

    buffer.pivot_->pos_ = p + 1 - buffer.pivot_->start_;
    r->headerState_ = HeaderState::START;

    return OK;

header_done:

    buffer.pivot_->pos_ = p + 1 - buffer.pivot_->start_;
    r->headerState_ = HeaderState::START;

    return DONE;
}

int parseChunked(std::shared_ptr<Request> r)
{
    u_char *pos, ch, c;
    int rc;
    int once = 0;
    int left;

    ChunkedInfo *ctx = &r->requestBody_.chunkedInfo_;
    auto &buffer = r->c_->readBuffer_;

    auto &state = ctx->state_;

    if (state == ChunkedState::DATA && ctx->size_ == 0)
    {
        state = ChunkedState::DATA_AFTER;
    }

    rc = AGAIN;

    // printf("%s\n", buffer.now->toString().c_str());
    // u_char *tmp = buffer.now->start + buffer.now->pos + ctx->data_offset;
    // if (tmp < buffer.now->start + buffer.now->len)
    // {
    //     printf("value:%d\n",*tmp);
    // }

    for (pos = buffer.pivot_->start_ + buffer.pivot_->pos_ + ctx->dataOffset_;
         pos < buffer.pivot_->start_ + buffer.pivot_->len_; pos++)
    {
        // printf("%d %d ", ctx->data_offset, *pos);
        once = 1;
        ch = *pos;

        switch (state)
        {

        case ChunkedState::START:
            if (ch >= '0' && ch <= '9')
            {
                state = ChunkedState::SIZE;
                ctx->size_ = ch - '0';
                break;
            }

            c = (u_char)(ch | 0x20);

            if (c >= 'a' && c <= 'f')
            {
                state = ChunkedState::SIZE;
                ctx->size_ = c - 'a' + 10;
                break;
            }
            goto invalid;

        case ChunkedState::SIZE:
            if (ctx->size_ > __LONG_LONG_MAX__ / 16)
            {
                goto invalid;
            }

            if (ch >= '0' && ch <= '9')
            {
                ctx->size_ = ctx->size_ * 16 + (ch - '0');
                break;
            }

            c = (u_char)(ch | 0x20);

            if (c >= 'a' && c <= 'f')
            {
                ctx->size_ = ctx->size_ * 16 + (c - 'a' + 10);
                break;
            }

            if (ctx->size_ == 0)
            {

                switch (ch)
                {
                case CR:
                    state = ChunkedState::LAST_EXTENSION_DONE;
                    break;
                case LF:
                    state = ChunkedState::TRAILER;
                    break;
                case ';':
                case ' ':
                case '\t':
                    state = ChunkedState::LAST_EXTENSION;
                    break;
                default:
                    goto invalid;
                }

                break;
            }

            switch (ch)
            {
            case CR:
                state = ChunkedState::EXTENSION_DONE;
                break;
            case LF:
                state = ChunkedState::DATA;
                break;
            case ';':
            case ' ':
            case '\t':
                state = ChunkedState::EXTENSION;
                break;
            default:
                goto invalid;
            }

            break;

        case ChunkedState::EXTENSION:
            switch (ch)
            {
            case CR:
                state = ChunkedState::EXTENSION_DONE;
                break;
            case LF:
                state = ChunkedState::DATA;
            }
            break;

        case ChunkedState::EXTENSION_DONE:
            if (ch == LF)
            {
                state = ChunkedState::DATA;
                break;
            }
            goto invalid;

        case ChunkedState::DATA:
            rc = OK;
            goto data;

        case ChunkedState::DATA_AFTER:
            switch (ch)
            {
            case CR:
                state = ChunkedState::DATA_AFTER_DONE;
                break;
            case LF:
                state = ChunkedState::START;
                break;
            default:
                goto invalid;
            }
            break;

        case ChunkedState::DATA_AFTER_DONE:
            if (ch == LF)
            {
                state = ChunkedState::START;
                break;
            }
            goto invalid;

        case ChunkedState::LAST_EXTENSION:
            switch (ch)
            {
            case CR:
                state = ChunkedState::LAST_EXTENSION_DONE;
                break;
            case LF:
                state = ChunkedState::TRAILER;
            }
            break;

        case ChunkedState::LAST_EXTENSION_DONE:
            if (ch == LF)
            {
                state = ChunkedState::TRAILER;
                break;
            }
            goto invalid;

        case ChunkedState::TRAILER:
            switch (ch)
            {
            case CR:
                state = ChunkedState::TRAILER_DONE;
                break;
            case LF:
                goto done;
            default:
                state = ChunkedState::TRAILER_HEADER;
            }
            break;

        case ChunkedState::TRAILER_DONE:
            if (ch == LF)
            {
                goto done;
            }
            goto invalid;

        case ChunkedState::TRAILER_HEADER:
            switch (ch)
            {
            case CR:
                state = ChunkedState::TRAILER_HEADER_DONE;
                break;
            case LF:
                state = ChunkedState::TRAILER;
            }
            break;

        case ChunkedState::TRAILER_HEADER_DONE:
            if (ch == LF)
            {
                state = ChunkedState::TRAILER;
                break;
            }
            goto invalid;
        }
    }

data:

    if (rc == OK)
    {
        ctx->dataOffset_ = ctx->size_;
    }

    if (rc == OK) // right after chunked size, we need to add the chunk size too!
    {
        // only add "SIZE\r\n", *pos supposed to be the first byte of data
        left = pos - buffer.pivot_->start_ - buffer.pivot_->pos_;
        r->requestBody_.listBody_.emplace_back(buffer.pivot_->start_ + buffer.pivot_->pos_, left);
        buffer.pivot_->pos_ += left;
    }
    else // add chunked data
    {
        if (once) // means this buffer contains all of this chunk, and the \r\n after
        {
            left = pos - buffer.pivot_->start_ - buffer.pivot_->pos_;
            ctx->dataOffset_ = 0;
        }
        else // chunk is larger than this buffer, just add them all
        {
            left = buffer.pivot_->len_ - buffer.pivot_->pos_;
            ctx->dataOffset_ -= left;
        }
        r->requestBody_.listBody_.emplace_back(buffer.pivot_->start_ + buffer.pivot_->pos_, left);
        buffer.pivot_->pos_ += left;
    }

    if (ctx->size_ > __LONG_LONG_MAX__ - 5)
    {
        goto invalid;
    }

    ctx->size_ = 0;

    return rc;

done:

    // *pos is the last \n
    left = pos + 1 - buffer.pivot_->start_ - buffer.pivot_->pos_;
    r->requestBody_.listBody_.emplace_back(buffer.pivot_->start_ + buffer.pivot_->pos_, left);

    // prepare for the next time
    ctx->state_ = ChunkedState::START;
    ctx->size_ = 0;
    ctx->dataOffset_ = 0;

    buffer.retrieve(left);

    return DONE;

invalid:

    return ERROR;
}

// HTTP/1.1 200 OK\r\n
int parseStatusLine(std::shared_ptr<Request> r, Status *status)
{
    u_char ch;
    u_char *p;

    auto &state = r->responseState_;
    auto &buffer = r->c_->readBuffer_;

    for (p = buffer.pivot_->start_ + buffer.pivot_->pos_; p < buffer.pivot_->start_ + buffer.pivot_->len_; p++)
    {
        ch = *p;

        switch (state)
        {

        case ResponseState::START:
            switch (ch)
            {
            case 'H':
                state = ResponseState::H;
                break;
            default:
                return ERROR;
            }
            break;

        case ResponseState::H:
            switch (ch)
            {
            case 'T':
                state = ResponseState::HT;
                break;
            default:
                return ERROR;
            }
            break;

        case ResponseState::HT:
            switch (ch)
            {
            case 'T':
                state = ResponseState::HTT;
                break;
            default:
                return ERROR;
            }
            break;

        case ResponseState::HTT:
            switch (ch)
            {
            case 'P':
                state = ResponseState::HTTP;
                break;
            default:
                return ERROR;
            }
            break;

        case ResponseState::HTTP:
            switch (ch)
            {
            case '/':
                state = ResponseState::MAJOR_DIGIT0;
                break;
            default:
                return ERROR;
            }
            break;

        // first digit in HTTP/1.1
        case ResponseState::MAJOR_DIGIT0:
            if (ch < '1' || ch > '9')
            {
                return ERROR;
            }

            r->httpMajor_ = ch - '0';
            state = ResponseState::MAJOR_DIGIT1;
            break;

        // dot
        case ResponseState::MAJOR_DIGIT1:
            if (ch == '.')
            {
                state = ResponseState::MINOR_DIGIT0;
                break;
            }

            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            if (r->httpMajor_ > 99)
            {
                return ERROR;
            }

            r->httpMajor_ = r->httpMajor_ * 10 + (ch - '0');
            break;

        // second digit in HTTP/1.1
        case ResponseState::MINOR_DIGIT0:
            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            r->httpMinor_ = ch - '0';
            state = ResponseState::MINOR_DIGIT1;
            break;

        case ResponseState::MINOR_DIGIT1:
            if (ch == ' ')
            {
                state = ResponseState::STATUS;
                break;
            }

            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            if (r->httpMinor_ > 99)
            {
                return ERROR;
            }

            r->httpMinor_ = r->httpMinor_ * 10 + (ch - '0');
            break;

        case ResponseState::STATUS:
            if (ch == ' ')
            {
                break;
            }

            if (ch < '0' || ch > '9')
            {
                return ERROR;
            }

            // response code has 3 digit
            if (++status->count_ == 3)
            {
                state = ResponseState::STATUS_SPACE;
                status->start_ = p - 2;
            }

            break;

        case ResponseState::STATUS_SPACE:
            switch (ch)
            {
            case ' ':
                state = ResponseState::STATUS_CONTENT;
                break;
            case '.':
                state = ResponseState::STATUS_CONTENT;
                break;
            case CR:
                state = ResponseState::RESPONSE_DONE;
                break;
            case LF:
                goto done;
            default:
                return ERROR;
            }
            break;

        case ResponseState::STATUS_CONTENT:
            switch (ch)
            {
            case CR:
                state = ResponseState::RESPONSE_DONE;
                break;
            case LF:
                goto done;
            }
            break;

        case ResponseState::RESPONSE_DONE:
            status->end_ = p - 1;
            switch (ch)
            {
            case LF:
                goto done;
            default:
                return ERROR;
            }
        }
    }

    buffer.pivot_->pos_ = p - buffer.pivot_->start_;

    return AGAIN;

done:

    buffer.pivot_->pos_ = p + 1 - buffer.pivot_->start_;

    if (status->end_ == NULL)
    {
        status->end_ = p;
    }

    status->httpVersion_ = r->httpMajor_ * 1000 + r->httpMinor_;
    state = ResponseState::START;

    return OK;
}