#include "../headers.h"

#include "http.h"
#include "http_parse.h"
#include "http_phases.h"

#include "../core/core.h"
#include "../event/event.h"
#include "../global.h"
#include "../log/logger.h"
#include "../utils/utils.h"

extern Server *serverPtr;
extern std::vector<Phase> phases;
extern std::unordered_set<char> normal;

std::unordered_map<int, std::string> httpCodeMap = {
    {HTTP_OK, "200 OK"},
    {HTTP_MOVED_PERMANENTLY, "301 Moved Permanently"},
    {HTTP_NOT_MODIFIED, "304 Not Modified"},
    {HTTP_UNAUTHORIZED, "401 Unauthorized"},
    {HTTP_FORBIDDEN, "403 Forbidden"},
    {HTTP_NOT_FOUND, "404 Not Found"},
    {HTTP_INTERNAL_SERVER_ERROR, "500 Internal Server Error"},
};

int acceptDelay(void *ev)
{
    Event *event = (Event *)ev;
    event->timeout_ = TimeoutStatus::NOT_TIMED_OUT;
    // use LT on listenfd
    if (serverPtr->multiplexer_->addFd(event->c_->fd_.get(), Events(IN), event->c_) == 0)
    {
        LOG_CRIT << "Listenfd add failed, errno:" << strerror(errno);
    }

    return 0;
}

int newConnection(Event *ev)
{
    while (1)
    {
        if (ev->timeout_ == TimeoutStatus::TIMEOUT)
        {
            return 1;
        }

        if (serverPtr->pool_.activeCnt >= serverConfig.connections)
        {
            if (serverConfig.eventDelay > 0)
            {
                ev->timeout_ = TimeoutStatus::TIMEOUT;
                serverPtr->timer_.add(ACCEPT_DELAY, "Accept delay", getTickMs() + serverConfig.eventDelay * 1000,
                                      acceptDelay, (void *)ev);
                if (serverPtr->multiplexer_->delFd(ev->c_->fd_.get()) != 1)
                {
                    LOG_CRIT << "Del accept event failed";
                }
            }
            return 1;
        }

        sockaddr_in addr;
        socklen_t len = sizeof(addr);
        Fd fd = accept4(ev->c_->fd_.get(), (sockaddr *)&addr, &len, SOCK_NONBLOCK);

        if (fd.get() < 0)
        {
            LOG_WARN << "Accept from fd:" << ev->c_->fd_.get() << " failed, errno: " << strerror(errno);
            return 1;
        }

        Connection *newc = serverPtr->pool_.getNewConnection();
        if (newc == NULL)
        {
            LOG_WARN << "Get new connection failed, listenfd:" << ev->c_->fd_.get();
            return 1;
        }

        newc->fd_.reset(std::move(fd));
        newc->addr_ = addr;
        newc->serverIdx_ = ev->c_->serverIdx_;

        newc->read_.handler_ = waitRequest;

        if (serverPtr->multiplexer_->addFd(newc->fd_.get(), Events(IN | ET), newc) == 0)
        {
            LOG_WARN << "Add client fd failed, fd:" << newc->fd_.get();
        }

        serverPtr->timer_.add(newc->fd_.get(), "Connection timeout", getTickMs() + 60000, setEventTimeout,
                              (void *)&newc->read_);

        LOG_INFO << "New connection from fd:" << ev->c_->fd_.get() << ", with fd:" << newc->fd_.get();
    }

    return 0;
}

int waitRequest(Event *ev)
{
    if (ev->timeout_ == TimeoutStatus::TIMEOUT)
    {
        LOG_INFO << "Client timeout, fd:" << ev->c_->fd_.get();
        finalizeConnection(ev->c_);
        return -1;
    }

    serverPtr->timer_.remove(ev->c_->fd_.get());

    Connection *c = ev->c_;

    int len = c->readBuffer_.bufferRecv(c->fd_.get(), 0);

    if (len == 0)
    {
        LOG_INFO << "Client close connection";
        finalizeConnection(c);
        return -1;
    }

    if (len < 0)
    {
        if (errno != EAGAIN)
        {
            LOG_WARN << "Read error: " << strerror(errno);
            finalizeConnection(c);
            return -1;
        }
        else
        {
            return 0;
        }
    }

    std::shared_ptr<Request> r(new Request());
    r->c_ = c;
    c->request_ = r;

    ev->handler_ = processRequestLine;
    processRequestLine(ev);
    return 0;
}

int waitRequestAgain(Event *ev)
{
    if (ev->timeout_ == TimeoutStatus::TIMEOUT)
    {
        LOG_INFO << "Client timeout, fd:" << ev->c_->fd_.get();
        finalizeRequest(ev->c_->request_);
        return -1;
    }

    serverPtr->timer_.remove(ev->c_->fd_.get());

    Connection *c = ev->c_;

    int len = c->readBuffer_.bufferRecv(c->fd_.get(), 0);

    if (len == 0)
    {
        LOG_INFO << "Client close connection";
        finalizeRequest(ev->c_->request_);
        return -1;
    }

    if (len < 0)
    {
        if (errno != EAGAIN)
        {
            LOG_WARN << "Read error: " << strerror(errno);
            finalizeRequest(ev->c_->request_);
            return -1;
        }
        else
        {
            return 0;
        }
    }

    ev->handler_ = processRequestLine;
    processRequestLine(ev);
    return 0;
}

int processRequestLine(Event *ev)
{
    // LOG_INFO << "process request line";
    Connection *c = ev->c_;
    std::shared_ptr<Request> r = c->request_;

    int ret = AGAIN;
    for (;;)
    {
        if (ret == AGAIN)
        {
            int rett = readRequest(r);
            if (rett == ERROR)
            {
                break;
            }
        }

        ret = parseRequestLine(r);
        if (ret == OK)
        {
            // the request line has been parsed successfully
            if (!r->c_->readBuffer_.atSameNode(r->requestStart_, r->requestEnd_))
            {
                LOG_WARN << "Request line too long";
                finalizeRequest(r);
                break;
            }

            r->requestLine_.len_ = r->requestEnd_ - r->requestStart_;
            r->requestLine_.data_ = r->requestStart_;

            LOG_INFO << "Request line:"
                     << std::string(r->requestLine_.data_, r->requestLine_.data_ + r->requestLine_.len_);

            if (handleRequestUri(r) != OK)
            {
                break;
            }

            ev->handler_ = processRequestHeaders;
            processRequestHeaders(ev);

            break;
        }

        if (ret != AGAIN)
        {
            LOG_WARN << "Parse request line failed";
            finalizeRequest(r);
            break;
        }

        if (r->c_->readBuffer_.pivot_->pos_ == r->c_->readBuffer_.pivot_->len_)
        {
            if (r->c_->readBuffer_.pivot_->next_)
            {
                r->c_->readBuffer_.pivot_ = r->c_->readBuffer_.pivot_->next_;
            }
        }
    }
    return OK;
}

int processRequestHeaders(Event *ev)
{
    int rc;
    Connection *c;
    std::shared_ptr<Request> r;

    c = ev->c_;
    r = c->request_;

    rc = AGAIN;

    for (;;)
    {
        if (rc == AGAIN)
        {
            if (r->c_->readBuffer_.pivot_->pos_ == r->c_->readBuffer_.pivot_->len_)
            {
                if (r->c_->readBuffer_.pivot_->next_)
                {
                    r->c_->readBuffer_.pivot_ = r->c_->readBuffer_.pivot_->next_;
                }
            }

            int ret = readRequest(r);
            if (ret == ERROR)
            {
                LOG_WARN << "Read request header failed";
                break;
            }
        }

        rc = parseHeaderLine(r);

        if (rc == OK)
        {
            if (r->invalidHeader_)
            {
                LOG_WARN << "Client sent invalid header line";
                continue;
            }

            if (r->c_->readBuffer_.tryMoveBuffer((void **)&r->headerNameStart_, (void **)&r->headerNameEnd_) == -1)
            {
                LOG_WARN << "Header name too long";
                finalizeRequest(r);
                break;
            }

            if (r->c_->readBuffer_.tryMoveBuffer((void **)&r->headerValueStart_, (void **)&r->headerValueEnd_) == -1)
            {
                LOG_WARN << "Header value too long";
                finalizeRequest(r);
                break;
            }

            // a header line has been parsed successfully
            r->contextIn_.headers_.emplace_back(std::string(r->headerNameStart_, r->headerNameEnd_),
                                                std::string(r->headerValueStart_, r->headerValueEnd_));

            Header &now = r->contextIn_.headers_.back();
            r->contextIn_.headerNameValueMap_[toLower(now.name_)] = now;

#ifdef LOG_HEADER
            LOG_INFO << now.name_ << ": " << now.value_;
#endif
            continue;
        }

        if (rc == DONE)
        {

            // all headers have been parsed successfully

            if (handleRequestHeader(r, 1) != OK)
            {
                finalizeRequest(r);
                break;
            }

            LOG_INFO << "Host: " << r->contextIn_.headerNameValueMap_["host"].value_;

            LOG_INFO << "Port: " << serverPtr->servers_[r->c_->serverIdx_].port;

            processRequest(r);

            break;
        }

        if (rc == AGAIN)
        {
            continue;
        }

        LOG_WARN << "Client sent invalid header line";
        finalizeRequest(r);
        return ERROR;
    }

    return OK;
}

int readRequest(std::shared_ptr<Request> r)
{
    Connection *c = r->c_;
    if (c == NULL)
    {
        LOG_WARN << "Connection is NULL";
        finalizeRequest(r);
        return ERROR;
    }

    int n;
    if (!c->readBuffer_.allRead())
    {
        return OK;
    }

    n = c->readBuffer_.bufferRecv(c->fd_.get(), 0);

    if (n < 0)
    {
        if (errno != EAGAIN)
        {
            LOG_WARN << "Read error: " << strerror(errno);
            finalizeRequest(r);
            return ERROR;
        }
        else
        {
            return AGAIN;
        }
    }
    else if (n == 0)
    {
        LOG_INFO << "Client close connection";
        finalizeRequest(r);
        return ERROR;
    }

    return OK;
}

int handleRequestUri(std::shared_ptr<Request> r)
{
    r->methodName_.len_ = r->methodEnd_ - r->requestStart_;
    r->methodName_.data_ = r->requestLine_.data_;

    if (r->protocol_.data_)
    {
        r->protocol_.len_ = r->requestEnd_ - r->protocol_.data_;
    }

    if (r->schemeEnd_)
    {
        r->scheme_.len_ = r->schemeEnd_ - r->schemeStart_;
        r->scheme_.data_ = r->schemeStart_;
    }

    if (r->hostEnd_)
    {
        r->host_.len_ = r->hostEnd_ - r->hostStart_;
        r->host_.data_ = r->hostStart_;
    }

    if (r->argsStart_)
    {
        r->uri_.len_ = r->argsStart_ - 1 - r->uriStart_;
    }
    else
    {
        r->uri_.len_ = r->uriEnd_ - r->uriStart_;
    }

    if (r->complexUri_ || r->quotedUri_ || r->emptyPathInUri_)
    {

        if (r->emptyPathInUri_)
        {
            r->uri_.len_++;
        }

        // free in Request::init()
        r->uri_.data_ = (u_char *)malloc(r->uri_.len_);

        if (handleComplexUri(r) != OK)
        {
            r->uri_.len_ = 0;
            LOG_WARN << "Client sent invalid request";
            finalizeRequest(r);
            return ERROR;
        }
    }
    else
    {
        r->uri_.data_ = r->uriStart_;
    }

    // true only if handleComplexUri isn't executed
    if (r->uriExtStart_)
    {
        if (r->argsStart_)
        {
            r->exten_.len_ = r->argsStart_ - 1 - r->uriExtStart_;
        }
        else
        {
            r->exten_.len_ = r->uriEnd_ - r->uriExtStart_;
        }

        r->exten_.data_ = r->uriExtStart_;
    }

    if (r->argsStart_ && r->uriEnd_ > r->argsStart_)
    {
        r->args_.len_ = r->uriEnd_ - r->argsStart_;
        r->args_.data_ = r->argsStart_;
    }

    LOG_INFO << "Uri:" << std::string(r->uri_.data_, r->uri_.data_ + r->uri_.len_);
    LOG_INFO << "Args:" << std::string(r->args_.data_, r->args_.data_ + r->args_.len_);
    LOG_INFO << "Exten:" << std::string(r->exten_.data_, r->exten_.data_ + r->exten_.len_);

    return OK;
}

// r->complexUri || r->quotedUri || r->emptyPathInUri
// already malloc a new space for uri.data
// 1. decode chars begin with '%' in QUOTED
// 2. extract exten in state like DOT
// 3. handle ./ and ../
int handleComplexUri(std::shared_ptr<Request> r)
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
    r->uriExtStart_ = NULL;
    r->argsStart_ = NULL;

    if (r->emptyPathInUri_)
    {
        // *now='/'; now++
        *now++ = '/';
    }

    ch = *old++;

    while (old <= r->uriEnd_)
    {
        switch (state)
        {

        case USUAL:

            if (normal.count(ch))
            {
                *now++ = ch;
                ch = *old++;
                break;
            }

            switch (ch)
            {
            case '/':
                r->uriExtStart_ = NULL;
                state = SLASH;
                *now++ = '/';
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
                *now++ = '.';
                r->uriExtStart_ = now;
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

            if (normal.count(ch))
            {
                state = USUAL;
                *now++ = ch;
                ch = *old++;
                break;
            }

            switch (ch)
            {
            case '/':
                // ignore multiple slashes
                break;
            case '.':
                state = DOT;
                *now++ = '.';
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

            if (normal.count(ch))
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
                *now++ = '.';
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

            if (normal.count(ch))
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
                // make now points to the place after the previous '/'
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

    if (r->uriExtStart_)
    {
        r->exten_.len_ = now - r->uriExtStart_;
        r->exten_.data_ = r->uriExtStart_;
    }

    r->uriExtStart_ = NULL;

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

    if (r->uriExtStart_)
    {
        r->exten_.len_ = now - r->uriExtStart_;
        r->exten_.data_ = r->uriExtStart_;
    }

    r->uriExtStart_ = NULL;

    return OK;
}

int handleRequestHeader(std::shared_ptr<Request> r, int needHost)
{
    auto &mp = r->contextIn_.headerNameValueMap_;

    if (mp.count("host"))
    {
    }
    else if (needHost)
    {
        LOG_WARN << "Client headers error";
        return ERROR;
    }

    if (mp.count("content-length"))
    {
        r->contextIn_.contentLength_ = atoi(mp["content-length"].value_.c_str());
    }
    else
    {
        r->contextIn_.contentLength_ = 0;
    }

    if (mp.count("transfer-encoding"))
    {
        r->contextIn_.isChunked_ = (0 == strcmp("chunked", mp["transfer-encoding"].value_.c_str()));
    }
    else
    {
        r->contextIn_.isChunked_ = 0;
    }

    if (mp.count("connection"))
    {
        auto &type = mp["connection"].value_;
        bool alive = (!strcmp("keep-alive", type.c_str())) || (!strcmp("Keep-Alive", type.c_str()));
        r->contextIn_.connectionType_ = alive ? ConnectionType::KEEP_ALIVE : ConnectionType::CLOSED;
    }
    else if (r->httpVersion_ > 1000)
    {
        r->contextIn_.connectionType_ = ConnectionType::KEEP_ALIVE;
    }
    else
    {
        r->contextIn_.connectionType_ = ConnectionType::CLOSED;
    }

    return OK;
}

int processRequest(std::shared_ptr<Request> r)
{
    Connection *c = r->c_;

    // read: empty; write: empty
    c->read_.handler_ = std::function<int(Event *)>();

    r->atPhase_ = 0;
    runPhases(&c->write_);
    return OK;
}

int processUpsStatusLine(std::shared_ptr<Request> upsr)
{
    std::shared_ptr<Upstream> ups = upsr->c_->upstream_;

    // OK AGAIN ERROR
    int ret = parseResponseLine(upsr, &ups->ctx_.status_);
    if (ret != OK)
    {
        return ret;
    }

    LOG_INFO << "OK";
    ups->processHandler_ = processUpsHeaders;
    return processUpsHeaders(upsr);
}

int processUpsHeaders(std::shared_ptr<Request> upsr)
{
    std::shared_ptr<Upstream> ups = upsr->c_->upstream_;

    while (1)
    {
        // OK AGAIN DONE ERROR
        int ret = parseHeaderLine(upsr);
        if (ret == OK)
        {
            if (upsr->invalidHeader_)
            {
                LOG_WARN << "Upstream sent invalid header line";
                continue;
            }

            if (upsr->c_->readBuffer_.tryMoveBuffer((void **)&upsr->headerNameStart_, (void **)&upsr->headerNameEnd_) ==
                -1)
            {
                LOG_WARN << "Upstream send too long header name";
                return ERROR;
            }

            if (upsr->c_->readBuffer_.tryMoveBuffer((void **)&upsr->headerValueStart_,
                                                    (void **)&upsr->headerValueEnd_) == -1)
            {
                LOG_WARN << "Upstream send too long header value";
                return ERROR;
            }

            upsr->contextIn_.headers_.emplace_back(std::string(upsr->headerNameStart_, upsr->headerNameEnd_),
                                                   std::string(upsr->headerValueStart_, upsr->headerValueEnd_));
            Header &now = upsr->contextIn_.headers_.back();
            upsr->contextIn_.headerNameValueMap_[toLower(now.name_)] = now;

            continue;
        }
        else if (ret == AGAIN)
        {
            if (upsr->c_->readBuffer_.pivot_->pos_ == upsr->c_->readBuffer_.pivot_->len_)
            {
                if (upsr->c_->readBuffer_.pivot_->next_)
                {
                    upsr->c_->readBuffer_.pivot_ = upsr->c_->readBuffer_.pivot_->next_;
                    continue;
                }
            }

            return AGAIN;
        }
        else if (ret == DONE)
        {
            if (handleRequestHeader(upsr, 0) != OK)
            {
                return ERROR;
            }

            upsr->requestBody_.left_ = -1;

            LOG_INFO << "OK";
            ups->processHandler_ = processUpsBody;
            return processUpsBody(upsr);
        }
        else
        {
            return ERROR;
        }
    }
}

int processUpsBody(std::shared_ptr<Request> upsr)
{
    int ret = 0;

    // no content-length && not chunked
    if (upsr->contextIn_.contentLength_ == 0 && !upsr->contextIn_.isChunked_)
    {
        LOG_INFO << "OK";
        return OK;
    }

    ret = processRequestBody(upsr);

    if (ret == OK)
    {
        LOG_INFO << "OK";
        return OK;
    }
    else
    {
        return ret;
    }
}

int runPhases(Event *ev)
{
    int ret = OK;
    std::shared_ptr<Request> r = ev->c_->request_;

    // OK: keep running phases
    // ERROR/DONE: quit phase running
    while ((size_t)r->atPhase_ < phases.size() && phases[r->atPhase_].phaseHandler)
    {
        ret = phases[r->atPhase_].phaseHandler(r, &phases[r->atPhase_]);
        if (ret == OK)
        {
            continue;
        }
        else
        {
            break;
        }
    }
    return ret;
}

int writeResponse(Event *ev)
{
    std::shared_ptr<Request> r = ev->c_->request_;
    auto &buffer = ev->c_->writeBuffer_;

    if (buffer.allRead() != 1)
    {
        int len = buffer.bufferSend(ev->c_->fd_.get(), 0);

        if (len < 0 && errno != EAGAIN)
        {
            LOG_WARN << "write response failed, errno: " << strerror(errno);
            finalizeRequest(r);
            return PHASE_ERR;
        }

        if (buffer.allRead() != 1)
        {
            r->c_->write_.handler_ = writeResponse;
            serverPtr->multiplexer_->modFd(ev->c_->fd_.get(), Events(IN | OUT | ET), ev->c_);
            return PHASE_QUIT;
        }
    }

    r->c_->write_.handler_ = std::function<int(Event *)>();
    serverPtr->multiplexer_->modFd(r->c_->fd_.get(), Events(IN | ET), r->c_);

    if (r->contextOut_.resType_ == ResponseType::FILE)
    {
        sendfileEvent(&r->c_->write_);
    }
    else if (r->contextOut_.resType_ == ResponseType::STRING)
    {
        sendStrEvent(&r->c_->write_);
    }
    else // RES_EMPTY
    {
        LOG_INFO << "Responsed";

        if (r->contextIn_.connectionType_ == ConnectionType::KEEP_ALIVE)
        {
            keepAliveRequest(r);
        }
        else
        {
            finalizeRequest(r);
        }
    }

    return PHASE_QUIT;
}

int clientAliveCheck(Event *ev)
{
    Connection *c = (Connection *)ev->c_;
    std::shared_ptr<Request> r = c->request_;
    Connection *upc = c->upstream_->upstream_;
    std::shared_ptr<Request> upsr = upc->request_;

    int n = c->readBuffer_.bufferRecv(c->fd_.get(), 0);

    if (n == 0)
    {
        LOG_INFO << "Client close connection";
        finalizeRequest(r);
        finalizeRequestLater(upsr);
        return OK;
    }

    if (n < 0 && errno != EAGAIN)
    {
        LOG_WARN << "Read error: " << strerror(errno);
        finalizeRequest(r);
        finalizeRequestLater(upsr);
        return ERROR;
    }

    LOG_WARN << "Unexpected result from client";
    finalizeRequest(r);
    finalizeRequestLater(upsr);
    return ERROR;
}

// @return OK AGAIN ERROR
int processRequestBody(std::shared_ptr<Request> r)
{
    if (r->contextIn_.isChunked_)
    {
        return processBodyChunked(r);
    }
    else
    {
        return processBodyLength(r);
    }
}

// @return OK AGAIN ERROR
int processBodyLength(std::shared_ptr<Request> r)
{
    auto &buffer = r->c_->readBuffer_;
    auto &rb = r->requestBody_;

    if (rb.left_ == -1)
    {
        rb.left_ = r->contextIn_.contentLength_;
    }

    while (buffer.allRead() != 1)
    {
        int len = buffer.pivot_->readableBytes();
        if (len <= rb.left_)
        {
            rb.left_ -= len;
            rb.listBody_.emplace_back(buffer.pivot_->start_ + buffer.pivot_->pos_, len);
            buffer.pivot_->pos_ = buffer.pivot_->len_;
        }
        else
        {
            LOG_WARN << "ERROR";
            return ERROR;
        }

        if (buffer.pivot_->readableBytes() == 0)
        {
            if (buffer.pivot_->next_ != NULL)
            {
                buffer.pivot_ = buffer.pivot_->next_;
            }
        }
    }

    if (rb.left_ == 0)
    {
        return OK;
    }
    else
    {
        return AGAIN;
    }
}

// @return OK AGAIN ERROR
int processBodyChunked(std::shared_ptr<Request> r)
{
    auto &buffer = r->c_->readBuffer_;
    auto &rb = r->requestBody_;

    int ret;

    while (1)
    {
        ret = parseChunked(r);

        if (ret == OK)
        {
            // a chunk has been parse successfully, keep going
            continue;
        }

        if (ret == DONE)
        {
            rb.left_ = 0;
            return OK;
        }

        if (ret == AGAIN)
        {
            if (buffer.pivot_->readableBytes() == 0)
            {
                if (buffer.pivot_->next_ != NULL)
                {
                    buffer.pivot_ = buffer.pivot_->next_;
                    continue;
                }
            }
            // need to recv more
            return AGAIN;
        }

        if (ret == ERROR)
        {
            // invalid
            LOG_WARN << "ERROR";
            return ERROR;
        }
    }

    LOG_WARN << "ERROR";
    return ERROR;
}

// @return OK AGAIN ERROR
int readRequestBody(std::shared_ptr<Request> r, std::function<int(std::shared_ptr<Request>)> postHandler)
{
    int ret = 0;

    // no content-length && not chunked
    if (r->contextIn_.contentLength_ == 0 && !r->contextIn_.isChunked_)
    {
        // read: empty; write: empty
        r->c_->read_.handler_ = std::function<int(Event *)>();
        if (postHandler)
        {
            postHandler(r);
        }
        return OK;
    }

    r->requestBody_.left_ = -1;
    ret = processRequestBody(r);

    if (ret == OK)
    {
        // read: empty; write: empty
        r->c_->read_.handler_ = std::function<int(Event *)>();
        if (postHandler)
        {
            postHandler(r);
        }
        return OK;
    }
    else if (ret == ERROR)
    {
        LOG_WARN << "ERROR";
        finalizeRequest(r);
        return ret;
    }

    // need recv again
    r->requestBody_.postHandler_ = postHandler;

    // read: readRequestBodyInner; write: empty
    r->c_->read_.handler_ = readRequestBodyInner;
    serverPtr->multiplexer_->modFd(r->c_->fd_.get(), Events(IN | ET), r->c_);

    return readRequestBodyInner(&r->c_->read_);
}

// @return OK AGAIN ERROR
int readRequestBodyInner(Event *ev)
{
    Connection *c = ev->c_;
    std::shared_ptr<Request> r = c->request_;

    auto &rb = r->requestBody_;
    auto &buffer = c->readBuffer_;

    while (1)
    {
        int ret = buffer.bufferRecv(c->fd_.get(), 0);

        if (ret == 0)
        {
            LOG_INFO << "Client close connection";
            finalizeRequest(r);
            return ERROR;
        }
        else if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                LOG_WARN << "EAGAIN";
                return AGAIN;
            }
            else
            {
                finalizeRequest(r);
                LOG_WARN << "Recv body error, errno: " << strerror(errno);
                return ERROR;
            }
        }
        else
        {
            ret = processRequestBody(r);

            if (ret == OK)
            {
                break;
            }
            else if (ret == ERROR)
            {
                finalizeRequest(r);
                return ERROR;
            }

            // AGAIN
            return ret;
        }
    }

    // read: empty; write: empty
    r->c_->read_.handler_ = std::function<int(Event *)>();
    if (rb.postHandler_)
    {
        LOG_INFO << "To post handler";
        rb.postHandler_(r);
    }

    return OK;
}

int sendfileEvent(Event *ev)
{
    std::shared_ptr<Request> r = ev->c_->request_;
    auto &filebody = r->contextOut_.fileBody_;

    ssize_t len =
        sendfile(r->c_->fd_.get(), filebody.filefd_.get(), &filebody.offset_, filebody.fileSize_ - filebody.offset_);

    if (len < 0 && errno != EAGAIN)
    {
        LOG_WARN << "Sendfile error: " << strerror(errno);
        finalizeRequest(r);
        return ERROR;
    }
    else if (len == 0)
    {
        LOG_INFO << "Client close connection";
        finalizeRequest(r);
        return ERROR;
    }

    if (filebody.fileSize_ - filebody.offset_ > 0)
    {
        r->c_->write_.handler_ = sendfileEvent;
        serverPtr->multiplexer_->modFd(r->c_->fd_.get(), Events(IN | OUT | ET), r->c_);
        return AGAIN;
    }

    serverPtr->multiplexer_->modFd(r->c_->fd_.get(), Events(IN | ET), r->c_);
    r->c_->write_.handler_ = std::function<int(Event *)>();
    filebody.filefd_.close();

    LOG_INFO << "Sendfile responsed";

    if (r->contextIn_.connectionType_ == ConnectionType::KEEP_ALIVE)
    {
        keepAliveRequest(r);
    }
    else
    {
        finalizeRequest(r);
    }

    return OK;
}

int sendStrEvent(Event *ev)
{
    std::shared_ptr<Request> r = ev->c_->request_;
    auto &strbody = r->contextOut_.strBody_;

    static int offset = 0;
    int bodysize = strbody.size();

    int len = send(r->c_->fd_.get(), strbody.c_str() + offset, bodysize - offset, 0);

    if (len < 0 && errno != EAGAIN)
    {
        LOG_WARN << "Send error: " << strerror(errno);
        finalizeRequest(r);
        return ERROR;
    }
    else if (len == 0)
    {
        LOG_INFO << "Client close connection";
        finalizeRequest(r);
        return ERROR;
    }

    offset += len;
    if (bodysize - offset > 0)
    {
        r->c_->write_.handler_ = sendStrEvent;
        serverPtr->multiplexer_->modFd(r->c_->fd_.get(), Events(IN | OUT | ET), r->c_);
        return AGAIN;
    }

    offset = 0;
    serverPtr->multiplexer_->modFd(r->c_->fd_.get(), Events(IN | ET), r->c_);
    r->c_->write_.handler_ = std::function<int(Event *)>();

    LOG_INFO << "Responsed";

    if (r->contextIn_.connectionType_ == ConnectionType::KEEP_ALIVE)
    {
        keepAliveRequest(r);
    }
    else
    {
        finalizeRequest(r);
    }

    return OK;
}

int keepAliveRequest(std::shared_ptr<Request> r)
{
    Connection *c = r->c_;

    r->init();

    r->c_ = c;

    c->read_.handler_ = waitRequestAgain;
    c->write_.handler_ = std::function<int(Event *)>();

    c->readBuffer_.init();
    c->writeBuffer_.init();

    serverPtr->timer_.add(c->fd_.get(), "Connection timeout", getTickMs() + 60000, setEventTimeout, (void *)&c->read_);

    return 0;
}

int timerRecoverConnection(void *arg)
{
    Connection *c = (Connection *)arg;

    if (serverPtr->pool_.isActive(c) && c->quit_)
    {
        int fd = c->fd_.get();
        serverPtr->multiplexer_->delFd(fd);
        serverPtr->pool_.recoverConnection(c);
        LOG_INFO << "Connection recover in timer, fd:" << fd;
    }

    return 0;
}

// close others
int finalizeConnectionLater(Connection *c)
{
    if (c == NULL)
    {
        LOG_CRIT << "Connection is NULL";
        return 0;
    }
    int fd = c->fd_.get();

    c->quit_ = 1;

    // delete c at timer
    serverPtr->timer_.add(fd, "finalize conn", getTickMs(), timerRecoverConnection, (void *)c);

    LOG_INFO << "Set connection->quit, fd:" << fd;
    return 0;
}

// close others
int finalizeRequestLater(std::shared_ptr<Request> r)
{
    r->quit_ = 1;
    finalizeConnectionLater(r->c_);
    return 0;
}

// close itself
int finalizeConnection(Connection *c)
{
    if (c == NULL)
    {
        LOG_CRIT << "Connection is NULL";
        return 0;
    }
    int fd = c->fd_.get();

    c->quit_ = 1;

    // delete c at the end of a eventloop

    LOG_INFO << "Set connection->quit, fd:" << fd;
    return 0;
}

// close itself
int finalizeRequest(std::shared_ptr<Request> r)
{
    r->quit_ = 1;
    finalizeConnection(r->c_);
    return 0;
}

std::string getEtag(int fd)
{
    if (fd < 0)
        return "";
    struct stat st;
    if (fstat(fd, &st) != 0)
        return "";
    std::string etag = std::to_string(st.st_mtim.tv_nsec + st.st_mtim.tv_sec * 1000);
    return etag;
}

bool etagMatched(int fd, std::string b_etag)
{
    if (fd < 0)
        return 0;
    struct stat st;
    if (fstat(fd, &st) != 0)
        return 0;

    std::string etag = std::to_string(st.st_mtim.tv_nsec + st.st_mtim.tv_sec * 1000);
    if (etag != b_etag)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void setErrorResponse(std::shared_ptr<Request> r, ResponseCode code)
{
    auto statStr = getByCode(code);
    r->contextOut_.resCode_ = code;
    r->contextOut_.statusLine_ = getStatusLineByCode(code);
    r->contextOut_.resType_ = ResponseType::STRING;
    auto &str = r->contextOut_.strBody_;
    str.append("<html>\n<head>\n\t<title>").append(statStr).append("</title>\n</head>\n");
    str.append("<body>\n\t<center>\n\t\t<h1>")
        .append(statStr)
        .append("</h1>\n\t</center>\n\t<hr>\n\t<center>MyServer</center>\n</body>\n</html>");

    r->contextOut_.headers_.emplace_back("Content-Type", getContentType("html", Charset::UTF_8));
    r->contextOut_.headers_.emplace_back("Content-Length", std::to_string(str.length()));
    r->contextOut_.headers_.emplace_back("Connection", "Keep-Alive");
}

std::string getByCode(ResponseCode code)
{
    if (httpCodeMap.count(code))
    {
        return httpCodeMap[code];
    }
    else
    {
        return httpCodeMap[HTTP_INTERNAL_SERVER_ERROR];
    }
}

std::string getStatusLineByCode(ResponseCode code)
{
    auto str = getByCode(code);
    return "HTTP/1.1 " + str + "\r\n";
}

std::string getContentType(std::string exten, Charset charset)
{
    std::string type;
    if (serverPtr->extenContentTypeMap_.count(exten))
    {
        type = serverPtr->extenContentTypeMap_[exten];
    }
    else
    {
        type = "application/octet-stream";
    }

    switch (charset)
    {
    case Charset::UTF_8:
        type.append("; charset=utf-8");
        break;
    case Charset::DEFAULT:
    default:
        break;
    }

    return type;
}

int selectServer(std::shared_ptr<Request> r)
{
    auto &server = serverPtr->servers_[r->c_->serverIdx_];
    int save = server.idx;
    server.idx = (server.idx + 1) % server.to.size();
    return save;
}