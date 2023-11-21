#include "../headers.h"

#include "http.h"
#include "http_parse.h"
#include "http_phases.h"

#include "../event/epoller.h"
#include "../event/event.h"
#include "../global.h"
#include "../utils/utils_declaration.h"

#include "../memory/memory_manage.hpp"

extern ConnectionPool cPool;
extern Server *serverPtr;
extern HeapMemory heap;
extern std::vector<PhaseHandler> phases;
extern std::unordered_map<std::string, std::string> extenContentTypeMap;

// std::unordered_map<std::string, std::string> extenContentTypeMap = {
//     {"html", "text/html"},
//     {"htm", "text/html"},
//     {"shtml", "text/html"},
//     {"css", "text/css"},
//     {"xml", "text/xml"},
//     {"gif", "image/gif"},
//     {"jpeg", "image/jpeg"},
//     {"jpg", "image/jpeg"},
//     {"js", "application/javascript"},
//     {"atom", "application/atom+xml"},
//     {"rss", "application/rss+xml"},
//     {"mml", "text/mathml"},
//     {"txt", "text/plain"},
//     {"jad", "text/vnd.sun.j2me.app-descriptor"},
//     {"wml", "text/vnd.wap.wml"},
//     {"htc", "text/x-component"},
//     {"avif", "image/avif"},
//     {"png", "image/png"},
//     {"svg", "image/svg+xml"},
//     {"svgz", "image/svg+xml"},
//     {"tif", "image/tiff"},
//     {"tiff", "image/tiff"},
//     {"wbmp", "image/vnd.wap.wbmp"},
//     {"webp", "image/webp"},
//     {"ico", "image/x-icon"},
//     {"jng", "image/x-jng"},
//     {"bmp", "image/x-ms-bmp"},
//     {"woff", "font/woff"},
//     {"woff2", "font/woff2"},
//     {"jar", "application/java-archive"},
//     {"war", "application/java-archive"},
//     {"ear", "application/java-archive"},
//     {"json", "application/json"},
//     {"hqx", "application/mac-binhex40"},
//     {"doc", "application/msword"},
//     {"pdf", "application/pdf"},
//     {"ps", "application/postscript"},
//     {"eps", "application/postscript"},
//     {"ai", "application/postscript"},
//     {"rtf", "application/rtf"},
//     {"m3u8", "application/vnd.apple.mpegurl"},
//     {"kml", "application/vnd.google-earth.kml+xml"},
//     {"kmz", "application/vnd.google-earth.kmz"},
//     {"xls", "application/vnd.ms-excel"},
//     {"eot", "application/vnd.ms-fontobject"},
//     {"ppt", "application/vnd.ms-powerpoint"},
//     {"odg", "application/vnd.oasis.opendocument.graphics"},
//     {"odp", "application/vnd.oasis.opendocument.presentation"},
//     {"ods", "application/vnd.oasis.opendocument.spreadsheet"},
//     {"odt", "application/vnd.oasis.opendocument.text"},
//     {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
//     {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
//     {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
//     {"wmlc", "application/vnd.wap.wmlc"},
//     {"wasm", "application/wasm"},
//     {"7z", "application/x-7z-compressed"},
//     {"cco", "application/x-cocoa"},
//     {"jardiff", "application/x-java-archive-diff"},
//     {"jnlp", "application/x-java-jnlp-file"},
//     {"run", "application/x-makeself"},
//     {"pl", "application/x-perl"},
//     {"pm", "application/x-perl"},
//     {"prc", "application/x-pilot"},
//     {"pdb", "application/x-pilot"},
//     {"rar", "application/x-rar-compressed"},
//     {"rpm", "application/x-redhat-package-manager"},
//     {"sea", "application/x-sea"},
//     {"swf", "application/x-shockwave-flash"},
//     {"sit", "application/x-stuffit"},
//     {"tcl", "application/x-tcl"},
//     {"tk", "application/x-tcl"},
//     {"der", "application/x-x509-ca-cert"},
//     {"pem", "application/x-x509-ca-cert"},
//     {"crt", "application/x-x509-ca-cert"},
//     {"xpi", "application/x-xpinstall"},
//     {"xhtml", "application/xhtml+xml"},
//     {"xspf", "application/xspf+xml"},
//     {"zip", "application/zip"},
//     {"bin", "application/octet-stream"},
//     {"exe", "application/octet-stream"},
//     {"dll", "application/octet-stream"},
//     {"deb", "application/octet-stream"},
//     {"dmg", "application/octet-stream"},
//     {"iso", "application/octet-stream"},
//     {"img", "application/octet-stream"},
//     {"msi", "application/octet-stream"},
//     {"msp", "application/octet-stream"},
//     {"msm", "application/octet-stream"},
//     {"mid", "audio/midi"},
//     {"midi", "audio/midi"},
//     {"kar", "audio/midi"},
//     {"mp3", "audio/mpeg"},
//     {"ogg", "audio/ogg"},
//     {"m4a", "audio/x-m4a"},
//     {"ra", "audio/x-realaudio"},
//     {"3gpp", "video/3gpp"},
//     {"3gp", "video/3gpp"},
//     {"ts", "video/mp2t"},
//     {"mp4", "video/mp4"},
//     {"mpeg", "video/mpeg"},
//     {"mpg", "video/mpeg"},
//     {"mov", "video/quicktime"},
//     {"webm", "video/webm"},
//     {"flv", "video/x-flv"},
//     {"m4v", "video/x-m4v"},
//     {"mng", "video/x-mng"},
//     {"asx", "video/x-ms-asf"},
//     {"asf", "video/x-ms-asf"},
//     {"wmv", "video/x-ms-wmv"},
//     {"avi", "video/x-msvideo"},
//     {"md", "text/markdown"},
// };

std::unordered_map<int, HttpCode> httpCodeMap = {
    {200, {200, "200 OK"}},
    {301, {301, "301 Moved Permanently"}},
    {304, {304, "304 Not Modified"}},
    {401, {401, "401 Unauthorized"}},
    {403, {403, "403 Forbidden"}},
    {404, {404, "404 Not Found"}},
    {500, {500, "500 Internal Server Error"}},
};

int initListen(Server *server, int port)
{
    Connection *listen = addListen(server, port);
    listen->serverIdx_ = server->listening_.size();

    server->listening_.push_back(listen);

    listen->read_.type_ = EventType::ACCEPT;
    listen->read_.handler_ = newConnection;

    return OK;
}

Connection *addListen(Server *server, int port)
{
    ConnectionPool *pool = server->pool_;
    Connection *listenConn = pool->getNewConnection();
    int reuse = 1;

    if (listenConn == NULL)
    {
        LOG_CRIT << "get listen failed";
        return NULL;
    }

    listenConn->addr_.sin_addr.s_addr = INADDR_ANY;
    listenConn->addr_.sin_family = AF_INET;
    listenConn->addr_.sin_port = htons(port);

    listenConn->fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenConn->fd_.getFd() < 0)
    {
        LOG_CRIT << "open listenfd failed";
        goto bad;
    }

    setsockopt(listenConn->fd_.getFd(), SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse));
    setNonblocking(listenConn->fd_.getFd());

    if (bind(listenConn->fd_.getFd(), (sockaddr *)&listenConn->addr_, sizeof(listenConn->addr_)) != 0)
    {
        LOG_CRIT << "bind failed";
        goto bad;
    }

    if (listen(listenConn->fd_.getFd(), 4096) != 0)
    {
        LOG_CRIT << "listen failed";
        goto bad;
    }

    LOG_INFO << "listen to " << port;

    return listenConn;

bad:
    pool->recoverConnection(listenConn);
    return NULL;
}

int newConnection(Event *ev)
{
#ifdef LOOP_ACCEPT
    while (1)
    {
#endif
        Connection *newc = cPool.getNewConnection();
        if (newc == NULL)
        {
            LOG_WARN << "Get new connection failed, listenfd:" << ev->c_->fd_.getFd();
            return 1;
        }

        sockaddr_in *addr = &newc->addr_;
        socklen_t len = sizeof(*addr);

        newc->fd_ = accept(ev->c_->fd_.getFd(), (sockaddr *)addr, &len);
        if (newc->fd_.getFd() < 0)
        {
            LOG_WARN << "Accept from FD:" << ev->c_->fd_.getFd() << " failed, errno: " << strerror(errno);
            cPool.recoverConnection(newc);
            return 1;
        }

        newc->serverIdx_ = ev->c_->serverIdx_;

        setNonblocking(newc->fd_.getFd());

        newc->read_.handler_ = waitRequest;

        if (serverPtr->multiplexer_->addFd(newc->fd_.getFd(), EVENTS(IN | ET), newc) == 0)
        {
            LOG_WARN << "Add client fd failed, FD:" << newc->fd_.getFd();
        }

        serverPtr->timer_.add(newc->fd_.getFd(), "Connection timeout", getTickMs() + 60000, setEventTimeout,
                              (void *)&newc->read_);

        LOG_INFO << "NEW CONNECTION FROM FD:" << ev->c_->fd_.getFd() << ", WITH FD:" << newc->fd_.getFd();
#ifdef LOOP_ACCEPT
    }
#endif

    return 0;
}

int waitRequest(Event *ev)
{
    if (ev->timeout_ == TimeoutStatus::TIMEOUT)
    {
        LOG_INFO << "Client timeout, FD:" << ev->c_->fd_.getFd();
        finalizeConnection(ev->c_);
        return -1;
    }

    serverPtr->timer_.remove(ev->c_->fd_.getFd());

    Connection *c = ev->c_;

    LOG_INFO << "waitRequest recv from FD:" << c->fd_.getFd();

    int len = c->readBuffer_.recvFd(c->fd_.getFd(), 0);

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
        LOG_INFO << "Client timeout, FD:" << ev->c_->fd_.getFd();
        finalizeRequest(ev->c_->request_);
        return -1;
    }

    serverPtr->timer_.remove(ev->c_->fd_.getFd());

    Connection *c = ev->c_;

    LOG_INFO << "keepAlive recv from FD:" << c->fd_.getFd();

    int len = c->readBuffer_.recvFd(c->fd_.getFd(), 0);

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
                LOG_WARN << "readRequestHeader ERROR";
                break;
            }
        }

        ret = parseRequestLine(r);
        if (ret == OK)
        {
            // the request line has been parsed successfully
            if (r->requestEnd_ < r->requestStart_ ||
                r->requestEnd_ > r->requestStart_ + r->c_->readBuffer_.now_->getSize())
            {
                LOG_WARN << "request line too long";
                finalizeRequest(r);
                break;
            }

            r->requestLine_.len_ = r->requestEnd_ - r->requestStart_;
            r->requestLine_.data_ = r->requestStart_;

            LOG_INFO << "request line:"
                     << std::string(r->requestLine_.data_, r->requestLine_.data_ + r->requestLine_.len_);

            r->methodName_.len_ = r->methodEnd_ - r->requestStart_ + 1;
            r->methodName_.data_ = r->requestLine_.data_;

            if (r->protocol_.data_)
            {
                r->protocol_.len_ = r->requestEnd_ - r->protocol_.data_;
            }

            if (handleRequestUri(r) != OK)
            {
                break;
            }

            if (r->schemaEnd_)
            {
                r->schema_.len_ = r->schemaEnd_ - r->schemaStart_;
                r->schema_.data_ = r->schemaStart_;
            }

            if (r->hostEnd_)
            {
                r->host_.len_ = r->hostEnd_ - r->hostStart_;
                r->host_.data_ = r->hostStart_;
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

        if (r->c_->readBuffer_.now_->pos_ == r->c_->readBuffer_.now_->len_)
        {
            if (r->c_->readBuffer_.now_->next_)
            {
                r->c_->readBuffer_.now_ = r->c_->readBuffer_.now_->next_;
            }
        }
    }
    return OK;
}

int tryMoveHeader(std::shared_ptr<Request> r, bool isName)
{
    u_char *left, *right;

    if (isName)
    {
        left = r->headerNameStart_;
        right = r->headerNameEnd_;
    }
    else
    {
        left = r->headerValueStart_;
        right = r->headerValueEnd_;
    }

    if (right < left || right > left + LinkedBufferNode::NODE_SIZE)
    {
        LinkedBufferNode *a = r->c_->readBuffer_.getNodeByAddr((uint64_t)left);
        LinkedBufferNode *b = r->c_->readBuffer_.getNodeByAddr((uint64_t)right);

        auto iter = r->c_->readBuffer_.nodes_.begin();
        size_t totalLen = 0;

        // check length
        {
            totalLen += a->start_ + a->len_ - left;
            totalLen += right - b->start_;
            LinkedBufferNode *now = a->next_;
            while (now != b)
            {
                totalLen += now->readableBytes();
                now = now->next_;
            }
            if (totalLen > LinkedBufferNode::NODE_SIZE)
            {
                return ERROR;
            }
        }

        while ((*iter) != (*b))
        {
            iter = std::next(iter);
        }

        // insert a new node, since we only need to make sure [start, end) is valid
        // we can also make it by "malloc"
        iter = r->c_->readBuffer_.insertNewNode(iter);
        iter->append(left, a->start_ + a->len_ - left);
        iter->append(b->start_, right - b->start_);

        if (isName)
        {
            r->headerNameStart_ = iter->start_ + iter->pos_;
            r->headerNameEnd_ = iter->start_ + iter->len_;
        }
        else
        {
            r->headerValueStart_ = iter->start_ + iter->pos_;
            r->headerValueEnd_ = iter->start_ + iter->len_;
        }

        iter->pos_ = iter->len_;
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

    LOG_INFO << "process request headers";

    rc = AGAIN;

    for (;;)
    {
        if (rc == AGAIN)
        {
            if (r->c_->readBuffer_.now_->pos_ == r->c_->readBuffer_.now_->len_)
            {
                if (r->c_->readBuffer_.now_->next_)
                {
                    r->c_->readBuffer_.now_ = r->c_->readBuffer_.now_->next_;
                }
            }

            int ret = readRequest(r);
            if (ret == ERROR)
            {
                LOG_WARN << "Read request header failed";
                break;
            }
        }

        rc = parseHeaderLine(r, 1);

        if (rc == OK)
        {
            if (r->invalidHeader_)
            {
                LOG_WARN << "Client sent invalid header line";
                continue;
            }

            if (tryMoveHeader(r, 1) == ERROR)
            {
                LOG_WARN << "Header name too long";
                finalizeRequest(r);
                break;
            }

            if (tryMoveHeader(r, 0) == ERROR)
            {
                LOG_WARN << "Header value too long";
                finalizeRequest(r);
                break;
            }

            // a header line has been parsed successfully
            r->inInfo_.headers_.emplace_back(std::string(r->headerNameStart_, r->headerNameEnd_),
                                             std::string(r->headerValueStart_, r->headerValueEnd_));

            Header &now = r->inInfo_.headers_.back();
            r->inInfo_.headerNameValueMap_[toLower(now.name_)] = now;

#ifdef LOG_HEADER
            LOG_INFO << now.name_ << ": " << now.value_;
#endif
            continue;
        }

        if (rc == DONE)
        {

            // all headers have been parsed successfully

            LOG_INFO << "http header done";

            rc = handleRequestHeader(r, 1);

            if (rc != OK)
            {
                break;
            }

            LOG_INFO << "Host: " << r->inInfo_.headerNameValueMap_["host"].value_;

            LOG_INFO << "Port: " << serverPtr->servers_[r->c_->serverIdx_].port_;

            processRequest(r);

            break;
        }

        if (rc == AGAIN)
        {

            // a header line parsing is still not complete

            continue;
        }

        LOG_WARN << "Client sent invalid header line";
        finalizeRequest(r);
        return ERROR;
        break;
    }

    return OK;
}

int readRequest(std::shared_ptr<Request> r)
{
    // LOG_INFO << "read request header";
    Connection *c = r->c_;
    if (c == NULL)
    {
        LOG_WARN << "connection is NULL";
        finalizeRequest(r);
        return ERROR;
    }

    int n;
    if (!c->readBuffer_.allRead())
    {
        return 1;
    }

    n = c->readBuffer_.recvFd(c->fd_.getFd(), 0);

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
    return n;
}

int handleRequestUri(std::shared_ptr<Request> r)
{
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
        r->uri_.data_ = (u_char *)heap.hMalloc(r->uri_.len_);

        if (parseComplexUri(r, 1) != OK)
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

    r->unparsedUri_.len_ = r->uriEnd_ - r->uriStart_;
    r->unparsedUri_.data_ = r->uriStart_;

    r->validUnparsedUri_ = r->emptyPathInUri_ ? 0 : 1;

    if (r->uriExt_)
    {
        if (r->argsStart_)
        {
            r->exten_.len_ = r->argsStart_ - 1 - r->uriExt_;
        }
        else
        {
            r->exten_.len_ = r->uriEnd_ - r->uriExt_;
        }

        r->exten_.data_ = r->uriExt_;
    }

    if (r->argsStart_ && r->uriEnd_ > r->argsStart_)
    {
        r->args_.len_ = r->uriEnd_ - r->argsStart_;
        r->args_.data_ = r->argsStart_;
    }

    LOG_INFO << "http uri:" << std::string(r->uri_.data_, r->uri_.data_ + r->uri_.len_);
    LOG_INFO << "http args:" << std::string(r->args_.data_, r->args_.data_ + r->args_.len_);
    LOG_INFO << "http exten:" << std::string(r->exten_.data_, r->exten_.data_ + r->exten_.len_);

    return OK;
}

int handleRequestHeader(std::shared_ptr<Request> r, int needHost)
{
    auto &mp = r->inInfo_.headerNameValueMap_;

    if (mp.count("host"))
    {
    }
    else if (needHost)
    {
        LOG_WARN << "Client headers error";
        finalizeRequest(r);
        return ERROR;
    }

    if (mp.count("content-length"))
    {
        r->inInfo_.contentLength_ = atoi(mp["content-length"].value_.c_str());
    }
    else
    {
        r->inInfo_.contentLength_ = 0;
    }

    if (mp.count("transfer-encoding"))
    {
        r->inInfo_.isChunked_ = (0 == strcmp("chunked", mp["transfer-encoding"].value_.c_str()));
    }
    else
    {
        r->inInfo_.isChunked_ = 0;
    }

    if (mp.count("connection"))
    {
        auto &type = mp["connection"].value_;
        bool alive = (!strcmp("keep-alive", type.c_str())) || (!strcmp("Keep-Alive", type.c_str()));
        r->inInfo_.connectionType_ = alive ? ConnectionType::KEEP_ALIVE : ConnectionType::CLOSED;
    }
    else if (r->httpVersion_ > 1000)
    {
        r->inInfo_.connectionType_ = ConnectionType::KEEP_ALIVE;
    }
    else
    {
        r->inInfo_.connectionType_ = ConnectionType::CLOSED;
    }

    return OK;
}

int processRequest(std::shared_ptr<Request> r)
{
    Connection *c = r->c_;
    c->read_.handler_ = blockReading;

    // // register EPOLLOUT
    // serverPtr->eventProccessor->modFd(c->fd_.getFd(), EPOLLIN | EPOLLOUT | EPOLLET, c);
    // c->write_.handler = runPhases;

    r->atPhase_ = 0;
    runPhases(&c->write_);
    return OK;
}

int processUpsStatusLine(std::shared_ptr<Request> upsr)
{
    std::shared_ptr<Upstream> ups = upsr->c_->upstream_;

    int ret = parseStatusLine(upsr, &ups->ctx_.status_);
    if (ret != OK)
    {
        return ret;
    }

    ups->processHandler_ = processUpsHeaders;
    return processUpsHeaders(upsr);
}

int processUpsHeaders(std::shared_ptr<Request> upsr)
{
    std::shared_ptr<Upstream> ups = upsr->c_->upstream_;
    int ret;

    while (1)
    {
        ret = parseHeaderLine(upsr, 1);
        if (ret == OK)
        {
            upsr->inInfo_.headers_.emplace_back(std::string(upsr->headerNameStart_, upsr->headerNameEnd_),
                                                std::string(upsr->headerValueStart_, upsr->headerValueEnd_));
            Header &now = upsr->inInfo_.headers_.back();
            upsr->inInfo_.headerNameValueMap_[toLower(now.name_)] = now;
            continue;
        }

        if (ret == AGAIN)
        {
            LOG_INFO << "AGAIN";
            return AGAIN;
        }

        if (ret == DONE)
        {
            LOG_INFO << "Upstream header done";
            handleRequestHeader(upsr, 0);

            ups->processHandler_ = processUpsBody;
            return processUpsBody(upsr);
        }
    }
}

int processUpsBody(std::shared_ptr<Request> upsr)
{
    int ret = 0;

    // no content-length && not chunked
    if (upsr->inInfo_.contentLength_ == 0 && !upsr->inInfo_.isChunked_)
    {
        LOG_INFO << "Upstream process body done";
        return OK;
    }

    upsr->requestBody_.left_ = -1;

    // for (auto &x : upsr->c->readBuffer_.nodes)
    // {
    //     if (x.len == 0)
    //         break;
    //     printf("%s", std::string(x.start, x.start + x.len).c_str());
    // }
    // printf("\n");

    ret = processRequestBody(upsr);

    if (ret == OK)
    {
        upsr->c_->read_.handler_ = blockReading;
        LOG_INFO << "Upstream process body done";
        return OK;
    }
    else
    {
        return ret;
    }
}

int runPhases(Event *ev)
{
    // auto &buffer = ev->c->readBuffer_;
    // for (auto &x : buffer.nodes)
    // {
    //     if (x.len == 0)
    //         break;
    //     printf("%s", std::string(x.start, x.start + x.len).c_str());
    // }
    LOG_INFO << "Phase running";

    int ret = OK;
    std::shared_ptr<Request> r = ev->c_->request_;

    // OK: keep running phases
    // ERROR/DONE: quit phase running
    while ((size_t)r->atPhase_ < phases.size() && phases[r->atPhase_].checker)
    {
        ret = phases[r->atPhase_].checker(r, &phases[r->atPhase_]);
        if (ret == OK)
        {
            continue;
        }
        else
        {
            break;
        }
    }
    LOG_INFO << "runPhases return:" << ret;
    return ret;
}

int writeResponse(Event *ev)
{
    std::shared_ptr<Request> r = ev->c_->request_;
    auto &buffer = ev->c_->writeBuffer_;

    if (buffer.allRead() != 1)
    {
        int len = buffer.sendFd(ev->c_->fd_.getFd(), 0);

        if (len < 0 && errno != EAGAIN)
        {
            LOG_WARN << "write response failed, errno: " << strerror(errno);
            finalizeRequest(r);
            return PHASE_ERR;
        }

        if (buffer.allRead() != 1)
        {
            r->c_->write_.handler_ = writeResponse;
            serverPtr->multiplexer_->modFd(ev->c_->fd_.getFd(), EVENTS(IN | OUT | ET), ev->c_);
            return PHASE_QUIT;
        }
    }

    r->c_->write_.handler_ = blockWriting;
    serverPtr->multiplexer_->modFd(r->c_->fd_.getFd(), EVENTS(IN | ET), r->c_);

    if (r->outInfo_.resType_ == ResponseType::FILE)
    {
        sendfileEvent(&r->c_->write_);
    }
    else if (r->outInfo_.resType_ == ResponseType::STRING)
    {
        sendStrEvent(&r->c_->write_);
    }
    else // RES_EMPTY
    {
        LOG_INFO << "RESPONSED";

        if (r->inInfo_.connectionType_ == ConnectionType::KEEP_ALIVE)
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

int blockReading(Event *ev)
{
    LOG_WARN << "Block reading triggered, FD:" << ev->c_->fd_.getFd();
    return OK;
}

int blockWriting(Event *ev)
{
    LOG_WARN << "Block writing triggered, FD:" << ev->c_->fd_.getFd();
    return OK;
}

// @return OK AGAIN ERROR
int processRequestBody(std::shared_ptr<Request> r)
{
    if (r->inInfo_.isChunked_)
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
        rb.left_ = r->inInfo_.contentLength_;
    }

    while (buffer.allRead() != 1)
    {
        int rlen = buffer.now_->len_ - buffer.now_->pos_;
        if (rlen <= rb.left_)
        {
            rb.left_ -= rlen;
            rb.listBody_.emplace_back(buffer.now_->start_ + buffer.now_->pos_, rlen);
            buffer.retrieve(rlen);
        }
        else
        {
            LOG_WARN << "ERROR";
            return ERROR;
        }

        if (buffer.now_->pos_ == buffer.now_->len_)
        {
            if (buffer.now_->next_ != NULL)
            {
                buffer.now_ = buffer.now_->next_;
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

    if (rb.left_ == -1)
    {
        // do nothing
    }

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
            if (buffer.now_->pos_ == buffer.now_->len_)
            {
                if (buffer.now_->next_ != NULL)
                {
                    buffer.now_ = buffer.now_->next_;
                    // since there are still data left, keep parsing
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
    if (r->inInfo_.contentLength_ == 0 && !r->inInfo_.isChunked_)
    {
        r->c_->read_.handler_ = blockReading;
        if (postHandler)
        {
            LOG_INFO << "To post_handler";
            postHandler(r);
        }
        return OK;
    }

    r->requestBody_.left_ = -1;
    r->requestBody_.postHandler_ = postHandler;

    ret = processRequestBody(r);

    if (ret == ERROR)
    {
        LOG_WARN << "ERROR";
        return ret;
    }

    r->c_->read_.handler_ = readRequestBodyInner;
    serverPtr->multiplexer_->modFd(r->c_->fd_.getFd(), EVENTS(IN | ET), r->c_);

    r->c_->write_.handler_ = blockWriting;

    ret = readRequestBodyInner(&r->c_->read_);

    return ret;
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
        if (rb.left_ == 0)
        {
            break;
        }

        int ret = buffer.recvFd(c->fd_.getFd(), 0);

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
                LOG_WARN << "Recv body error";
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

            return ret;
        }
    }

    r->c_->read_.handler_ = blockReading;
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
    auto &filebody = r->outInfo_.fileBody_;

    ssize_t len = sendfile(r->c_->fd_.getFd(), filebody.filefd_.getFd(), &filebody.offset_,
                           filebody.fileSize_ - filebody.offset_);

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
        serverPtr->multiplexer_->modFd(r->c_->fd_.getFd(), EVENTS(IN | OUT | ET), r->c_);
        return AGAIN;
    }

    serverPtr->multiplexer_->modFd(r->c_->fd_.getFd(), EVENTS(IN | ET), r->c_);
    r->c_->write_.handler_ = blockWriting;
    filebody.filefd_.closeFd();

    LOG_INFO << "SENDFILE RESPONSED";

    if (r->inInfo_.connectionType_ == ConnectionType::KEEP_ALIVE)
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
    auto &strbody = r->outInfo_.strBody_;

    static int offset = 0;
    int bodysize = strbody.size();

    int len = send(r->c_->fd_.getFd(), strbody.c_str() + offset, bodysize - offset, 0);

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
        serverPtr->multiplexer_->modFd(r->c_->fd_.getFd(), EVENTS(IN | OUT | ET), r->c_);
        return AGAIN;
    }

    offset = 0;
    serverPtr->multiplexer_->modFd(r->c_->fd_.getFd(), EVENTS(IN | ET), r->c_);
    r->c_->write_.handler_ = blockWriting;

    LOG_INFO << "SENDSTR RESPONSED";

    if (r->inInfo_.connectionType_ == ConnectionType::KEEP_ALIVE)
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
    int fd = r->c_->fd_.getFd();

    Connection *c = r->c_;

    r->init();

    r->c_ = c;

    c->read_.handler_ = waitRequestAgain;
    c->write_.handler_ = std::function<int(Event *)>();

    c->readBuffer_.init();
    c->writeBuffer_.init();

    serverPtr->timer_.add(c->fd_.getFd(), "Connection timeout", getTickMs() + 60000, setEventTimeout,
                          (void *)&c->read_);

    LOG_INFO << "KEEPALIVE CONNECTION DONE, FD:" << fd;

    return 0;
}

int finalizeConnection(Connection *c)
{
    if (c == NULL)
    {
        LOG_CRIT << "FINALIZE CONNECTION NULL";
        return 0;
    }
    int fd = c->fd_.getFd();

    c->quit_ = 1;

    // delete c at the end of a eventloop
    // cPool.recoverConnection(c);

    LOG_INFO << "set connection->quit, FD:" << fd;
    return 0;
}

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

bool matchEtag(int fd, std::string b_etag)
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
    HttpCode hc = getByCode(code);
    r->outInfo_.resCode_ = code;
    r->outInfo_.statusLine_ = getStatusLineByCode(code);
    r->outInfo_.resType_ = ResponseType::STRING;
    auto &str = r->outInfo_.strBody_;
    str.append("<html>\n<head>\n\t<title>").append(hc.str_).append("</title>\n</head>\n");
    str.append("<body>\n\t<center>\n\t\t<h1>")
        .append(hc.str_)
        .append("</h1>\n\t</center>\n\t<hr>\n\t<center>MyServer</center>\n</body>\n</html>");

    r->outInfo_.headers_.emplace_back("Content-Type", getContentType("html", Charset::UTF_8));
    r->outInfo_.headers_.emplace_back("Content-Length", std::to_string(str.length()));
    r->outInfo_.headers_.emplace_back("Connection", "Keep-Alive");
}

HttpCode getByCode(ResponseCode code)
{
    HttpCode ans;
    switch (code)
    {
    case ResponseCode::HTTP_OK:
        ans = httpCodeMap[200];
        break;
    case ResponseCode::HTTP_NOT_MODIFIED:
        ans = httpCodeMap[304];
        break;
    case ResponseCode::HTTP_UNAUTHORIZED:
        ans = httpCodeMap[401];
        break;
    case ResponseCode::HTTP_FORBIDDEN:
        ans = httpCodeMap[403];
        break;
    case ResponseCode::HTTP_NOT_FOUND:
        ans = httpCodeMap[404];
        break;
    case ResponseCode::HTTP_MOVED_PERMANENTLY:
        ans = httpCodeMap[301];
        break;
    case ResponseCode::HTTP_INTERNAL_SERVER_ERROR:
        // fall through
    default:
        ans = httpCodeMap[500];
        break;
    }

    return ans;
}

std::string getStatusLineByCode(ResponseCode code)
{
    HttpCode hc = getByCode(code);
    return "HTTP/1.1 " + hc.str_ + "\r\n";
}

std::string getContentType(std::string exten, Charset charset)
{
    std::string type;
    if (extenContentTypeMap.count(exten))
    {
        type = extenContentTypeMap[exten];
    }
    else
    {
        type = extenContentTypeMap["a_default_content_type"];
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