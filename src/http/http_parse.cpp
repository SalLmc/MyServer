#include "../headers.h"

#include "../core/core.h"
#include "http.h"
#include "http_parse.h"

std::unordered_set<char> normal = {'A', 'B', 'C', 'D',  'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                   'Q', 'R', 'S', 'T',  'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                   'g', 'h', 'i', 'j',  'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                   'w', 'x', 'y', 'z',  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_',
                                   '~', '!', '*', '\'', '(', ')', ';', ':', '@', '&', '=', '$', ',', '[', ']'};

u_char lowcase[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                          "\0\0\0\0\0\0\0\0\0\0\0\0\0-\0\0"
                          "0123456789\0\0\0\0\0\0"
                          "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0_"
                          "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
                          "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                          "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                          "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                          "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

// GET /example/path HTTP/1.1\r\n
int parseRequestLine(std::shared_ptr<Request> r)
{
    // LOG_INFO << "parse request line";
    u_char c, ch, *p, *m;

    auto &state = r->requestState_;
    auto &buffer = r->c_->readBuffer_;

    static std::unordered_map<std::string, Method> strMethodMap = {
        {"GET", Method::GET},           {"POST", Method::POST},
        {"PUT", Method::PUT},           {"DELETE", Method::DELETE},
        {"COPY", Method::COPY},         {"MOVE", Method::MOVE},
        {"LOCK", Method::LOCK},         {"HEAD", Method::HEAD},
        {"MKCOL", Method::MKCOL},       {"PATCH", Method::PATCH},
        {"TRACE", Method::TRACE},       {"UNLOCK", Method::UNLOCK},
        {"OPTIONS", Method::OPTIONS},   {"CONNECT", Method::CONNECT},
        {"PROPFIND", Method::PROPFIND}, {"PROPPATCH", Method::PROPPATCH},
    };

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

            if (ch < 'A' || ch > 'Z')
            {
                return ERROR;
            }

            state = RequestState::METHOD;

            break;

        case RequestState::METHOD:
            // continue until we meet the space after method
            if (ch == ' ')
            {
                r->methodEnd_ = p;
                m = r->requestStart_;

                std::string method(m, p);
                if (strMethodMap.count(method))
                {
                    r->method_ = strMethodMap[method];
                }
                else
                {
                    return ERROR;
                }

                state = RequestState::AFTER_METHOD;

                break;
            }

            if (ch < 'A' || ch > 'Z')
            {
                return ERROR;
            }

            break;

        // GET /example/path HTTP/1.1\r\n
        case RequestState::AFTER_METHOD:

            if (ch == '/')
            {
                r->uriStart_ = p;
                state = RequestState::URI_AFTER_SLASH;
                break;
            }

            // turn ch to lowercase
            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'z')
            {
                r->schemeStart_ = p;
                state = RequestState::SCHEME;
                break;
            }

            // skip space
            if (ch == ' ')
            {
                break;
            }
            else
            {
                return ERROR;
            }

            break;

        // http, ftp
        case RequestState::SCHEME:

            c = (u_char)(ch | 0x20);
            if (c >= 'a' && c <= 'z')
            {
                break;
            }

            if (ch == ':')
            {
                r->schemeEnd_ = p;
                state = RequestState::SCHEME_SLASH0;
                break;
            }
            else
            {
                return ERROR;
            }

            break;

        case RequestState::SCHEME_SLASH0:

            if (ch == '/')
            {
                state = RequestState::SCHEME_SLASH1;
                break;
            }
            else
            {
                return ERROR;
            }

            break;

        case RequestState::SCHEME_SLASH1:

            if (ch == '/')
            {
                state = RequestState::HOST_START;
                break;
            }
            else
            {
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

            if ((c >= 'a' && c <= 'z') || (ch >= '0' && ch <= '9') || ch == '.')
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
                state = RequestState::URI_AFTER_SLASH;
                break;
            case '?':
                r->uriStart_ = p;
                r->argsStart_ = p + 1;
                r->emptyPathInUri_ = 1;
                state = RequestState::URI;
                break;
            case ' ':
                // use single "/" from request line to preserve pointers
                // in case this request line will be copied to another buffer
                // since schemeEnd_+1 points to an '/'. like: http://
                r->uriStart_ = r->schemeEnd_ + 1;
                r->uriEnd_ = r->schemeEnd_ + 2;
                state = RequestState::HTTP_START;
                break;
            default:
                return ERROR;
            }
            break;

        case RequestState::HOST_IP:

            c = (u_char)(ch | 0x20);

            if ((c >= 'a' && c <= 'z') || (ch >= '0' && ch <= '9'))
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
                r->uriStart_ = p;
                state = RequestState::URI_AFTER_SLASH;
                break;
            case '?':
                r->uriStart_ = p;
                r->argsStart_ = p + 1;
                r->emptyPathInUri_ = 1;
                state = RequestState::URI;
                break;
            case ' ':
                // same as before
                r->uriStart_ = r->schemeEnd_ + 1;
                r->uriEnd_ = r->schemeEnd_ + 2;
                state = RequestState::HTTP_START;
                break;
            default:
                return ERROR;
            }

            break;

        case RequestState::URI_AFTER_SLASH:

            if (normal.count(ch))
            {
                state = RequestState::URI_EXT_CHECK;
                break;
            }

            switch (ch)
            {
            case ' ':
                r->uriEnd_ = p;
                state = RequestState::HTTP_START;
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
                if (ch < 32 || ch >= 127)
                {
                    return ERROR;
                }
                state = RequestState::URI_EXT_CHECK;
                break;
            }
            break;

        case RequestState::URI_EXT_CHECK:

            if (normal.count(ch))
            {
                break;
            }

            switch (ch)
            {
            case ' ':
                r->uriEnd_ = p;
                state = RequestState::HTTP_START;
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
                r->uriExtStart_ = p + 1;
                break;
            case '%':
                r->quotedUri_ = 1;
                state = RequestState::URI;
                break;
            case '/':
                r->uriExtStart_ = NULL;
                state = RequestState::URI_AFTER_SLASH;
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
                if (ch < 32 || ch >= 127)
                {
                    return ERROR;
                }
                break;
            }
            break;

        // ".", '?', '%', '/', '#', '+'
        case RequestState::URI:

            if (normal.count(ch))
            {
                break;
            }

            switch (ch)
            {
            case ' ':
                r->uriEnd_ = p;
                state = RequestState::HTTP_START;
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
                if (ch < 32 || ch >= 127)
                {
                    return ERROR;
                }
                break;
            }
            break;

        case RequestState::HTTP_START:
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

// Host: www.example.com\r\n
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n
// Accept-Language: en-US,en;q=0.5\r\n
// Connection: keep-alive\r\n
// \r\n
int parseHeaderLine(std::shared_ptr<Request> r)
{
    u_char c, ch, *p;

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

                // header can't contain space
                if (ch <= 32 || ch >= 127 || ch == ':')
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

            if (ch <= 32 || ch >= 127)
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
                // this space is a part of a header's value
                state = HeaderState::VALUE;
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
int parseResponseLine(std::shared_ptr<Request> r, UpsResInfo *status)
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
                status->start_ = p;
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

            // response code has 3 digits
            if (++status->count_ == 3)
            {
                state = ResponseState::STATUS_SPACE;
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

    r->httpVersion_ = r->httpMajor_ * 1000 + r->httpMinor_;
    state = ResponseState::START;

    return OK;
}