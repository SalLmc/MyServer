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
extern Cycle *cyclePtr;
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
    {200, {200, "200 OK"}},        {304, {304, "304 Not Modified"}}, {401, {401, "401 Unauthorized"}},
    {403, {403, "403 Forbidden"}}, {404, {404, "404 Not Found"}},    {500, {500, "500 Internal Server Error"}},

};

int initListen(Cycle *cycle, int port)
{
    Connection *listen = addListen(cycle, port);
    listen->serverIdx_ = cycle->listening_.size();

    cycle->listening_.push_back(listen);

    listen->read_.type = ACCEPT;
    listen->read_.handler = newConnection;

    return OK;
}

Connection *addListen(Cycle *cycle, int port)
{
    ConnectionPool *pool = cycle->pool_;
    Connection *listenC = pool->getNewConnection();
    int reuse = 1;

    if (listenC == NULL)
    {
        LOG_CRIT << "get listen failed";
        return NULL;
    }

    listenC->addr_.sin_addr.s_addr = INADDR_ANY;
    listenC->addr_.sin_family = AF_INET;
    listenC->addr_.sin_port = htons(port);

    listenC->fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenC->fd_.getFd() < 0)
    {
        LOG_CRIT << "open listenfd failed";
        goto bad;
    }

    setsockopt(listenC->fd_.getFd(), SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse));
    setNonblocking(listenC->fd_.getFd());

    if (bind(listenC->fd_.getFd(), (sockaddr *)&listenC->addr_, sizeof(listenC->addr_)) != 0)
    {
        LOG_CRIT << "bind failed";
        goto bad;
    }

    if (listen(listenC->fd_.getFd(), 4096) != 0)
    {
        LOG_CRIT << "listen failed";
        goto bad;
    }

    LOG_INFO << "listen to " << port;

    return listenC;

bad:
    pool->recoverConnection(listenC);
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
            LOG_WARN << "Get new connection failed, listenfd:" << ev->c->fd_.getFd();
            return 1;
        }

        sockaddr_in *addr = &newc->addr_;
        socklen_t len = sizeof(*addr);

        newc->fd_ = accept(ev->c->fd_.getFd(), (sockaddr *)addr, &len);
        if (newc->fd_.getFd() < 0)
        {
            LOG_WARN << "Accept from FD:" << ev->c->fd_.getFd() << " failed, errno: " << strerror(errno);
            cPool.recoverConnection(newc);
            return 1;
        }

        newc->serverIdx_ = ev->c->serverIdx_;

        setNonblocking(newc->fd_.getFd());

        newc->read_.handler = waitRequest;

        if (cyclePtr->multiplexer->addFd(newc->fd_.getFd(), EVENTS(IN | ET), newc) == 0)
        {
            LOG_WARN << "Add client fd failed, FD:" << newc->fd_.getFd();
        }

        cyclePtr->timer_.Add(newc->fd_.getFd(), getTickMs() + 60000, setEventTimeout, (void *)&newc->read_);

        LOG_INFO << "NEW CONNECTION FROM FD:" << ev->c->fd_.getFd() << ", WITH FD:" << newc->fd_.getFd();
#ifdef LOOP_ACCEPT
    }
#endif

    return 0;
}

int waitRequest(Event *ev)
{
    if (ev->timeout == TIMEOUT)
    {
        LOG_INFO << "Client timeout, FD:" << ev->c->fd_.getFd();
        finalizeConnection(ev->c);
        return -1;
    }

    cyclePtr->timer_.Remove(ev->c->fd_.getFd());

    Connection *c = ev->c;

    LOG_INFO << "waitRequest recv from FD:" << c->fd_.getFd();

    int len = c->readBuffer_.cRecvFd(c->fd_.getFd(), &errno, 0);

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
    r->c = c;
    c->request_ = r;

    ev->handler = processRequestLine;
    processRequestLine(ev);
    return 0;
}

int waitRequestAgain(Event *ev)
{
    if (ev->timeout == TIMEOUT)
    {
        LOG_INFO << "Client timeout, FD:" << ev->c->fd_.getFd();
        finalizeRequest(ev->c->request_);
        return -1;
    }

    cyclePtr->timer_.Remove(ev->c->fd_.getFd());

    Connection *c = ev->c;

    LOG_INFO << "keepAlive recv from FD:" << c->fd_.getFd();

    int len = c->readBuffer_.cRecvFd(c->fd_.getFd(), &errno, 0);

    if (len == 0)
    {
        LOG_INFO << "Client close connection";
        finalizeRequest(ev->c->request_);
        return -1;
    }

    if (len < 0)
    {
        if (errno != EAGAIN)
        {
            LOG_WARN << "Read error: " << strerror(errno);
            finalizeRequest(ev->c->request_);
            return -1;
        }
        else
        {
            return 0;
        }
    }

    ev->handler = processRequestLine;
    processRequestLine(ev);
    return 0;
}

int processRequestLine(Event *ev)
{
    // LOG_INFO << "process request line";
    Connection *c = ev->c;
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
            if (r->requestEnd < r->requestStart || r->requestEnd > r->requestStart + r->c->readBuffer_.now->getSize())
            {
                LOG_WARN << "request line too long";
                finalizeRequest(r);
                break;
            }

            r->requestLine.len = r->requestEnd - r->requestStart;
            r->requestLine.data = r->requestStart;
            r->requestLength = r->c->readBuffer_.now->start + r->c->readBuffer_.now->pos - r->requestStart;

            LOG_INFO << "request line:" << std::string(r->requestLine.data, r->requestLine.data + r->requestLine.len);

            r->methodName.len = r->methodEnd - r->requestStart + 1;
            r->methodName.data = r->requestLine.data;

            if (r->protocol.data)
            {
                r->protocol.len = r->requestEnd - r->protocol.data;
            }

            if (handleRequestUri(r) != OK)
            {
                break;
            }

            if (r->schemaEnd)
            {
                r->schema.len = r->schemaEnd - r->schemaStart;
                r->schema.data = r->schemaStart;
            }

            if (r->hostEnd)
            {
                r->host.len = r->hostEnd - r->hostStart;
                r->host.data = r->hostStart;
            }

            ev->handler = processRequestHeaders;
            processRequestHeaders(ev);

            break;
        }

        if (ret != AGAIN)
        {
            LOG_WARN << "Parse request line failed";
            finalizeRequest(r);
            break;
        }

        if (r->c->readBuffer_.now->pos == r->c->readBuffer_.now->len)
        {
            if (r->c->readBuffer_.now->next)
            {
                r->c->readBuffer_.now = r->c->readBuffer_.now->next;
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

    c = ev->c;
    r = c->request_;

    LOG_INFO << "process request headers";

    rc = AGAIN;

    for (;;)
    {
        if (rc == AGAIN)
        {
            if (r->c->readBuffer_.now->pos == r->c->readBuffer_.now->len)
            {
                if (r->c->readBuffer_.now->next)
                {
                    r->c->readBuffer_.now = r->c->readBuffer_.now->next;
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

            r->requestLength += r->c->readBuffer_.now->start + r->c->readBuffer_.now->pos - r->headerNameStart;

            if (r->invalidHeader)
            {
                LOG_WARN << "Client sent invalid header line";
                continue;
            }

            // a header line has been parsed successfully
            r->inInfo.headers.emplace_back(std::string(r->headerNameStart, r->headerNameEnd),
                                           std::string(r->headerValueStart, r->headerValueEnd));

            Header &now = r->inInfo.headers.back();
            r->inInfo.headerNameValueMap[toLower(now.name)] = now;

#ifdef LOG_HEADER
            LOG_INFO << now.name << ": " << now.value;
#endif
            continue;
        }

        if (rc == DONE)
        {

            // all headers have been parsed successfully

            LOG_INFO << "http header done";

            r->requestLength += r->c->readBuffer_.now->start + r->c->readBuffer_.now->pos - r->headerNameStart;

            rc = handleRequestHeader(r, 1);

            if (rc != OK)
            {
                break;
            }

            LOG_INFO << "Host: " << r->inInfo.headerNameValueMap["host"].value;

            LOG_INFO << "Port: " << cyclePtr->servers_[r->c->serverIdx_].port;

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
    Connection *c = r->c;
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

    n = c->readBuffer_.cRecvFd(c->fd_.getFd(), &errno, 0);

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
    if (r->argsStart)
    {
        r->uri.len = r->argsStart - 1 - r->uriStart;
    }
    else
    {
        r->uri.len = r->uriEnd - r->uriStart;
    }

    if (r->complexUri || r->quotedUri || r->emptyPathInUri)
    {

        if (r->emptyPathInUri)
        {
            r->uri.len++;
        }

        // free in Request::init()
        r->uri.data = (u_char *)heap.hMalloc(r->uri.len);

        if (parseComplexUri(r, 1) != OK)
        {
            r->uri.len = 0;
            LOG_WARN << "Client sent invalid request";
            finalizeRequest(r);
            return ERROR;
        }
    }
    else
    {
        r->uri.data = r->uriStart;
    }

    r->unparsedUri.len = r->uriEnd - r->uriStart;
    r->unparsedUri.data = r->uriStart;

    r->validUnparsedUri = r->emptyPathInUri ? 0 : 1;

    if (r->uriExt)
    {
        if (r->argsStart)
        {
            r->exten.len = r->argsStart - 1 - r->uriExt;
        }
        else
        {
            r->exten.len = r->uriEnd - r->uriExt;
        }

        r->exten.data = r->uriExt;
    }

    if (r->argsStart && r->uriEnd > r->argsStart)
    {
        r->args.len = r->uriEnd - r->argsStart;
        r->args.data = r->argsStart;
    }

    LOG_INFO << "http uri:" << std::string(r->uri.data, r->uri.data + r->uri.len);
    LOG_INFO << "http args:" << std::string(r->args.data, r->args.data + r->args.len);
    LOG_INFO << "http exten:" << std::string(r->exten.data, r->exten.data + r->exten.len);

    return OK;
}

int handleRequestHeader(std::shared_ptr<Request> r, int needHost)
{
    auto &mp = r->inInfo.headerNameValueMap;

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
        r->inInfo.contentLength = atoi(mp["content-length"].value.c_str());
    }
    else
    {
        r->inInfo.contentLength = 0;
    }

    if (mp.count("transfer-encoding"))
    {
        r->inInfo.isChunked = (0 == strcmp("chunked", mp["transfer-encoding"].value.c_str()));
    }
    else
    {
        r->inInfo.isChunked = 0;
    }

    if (mp.count("connection"))
    {
        auto &type = mp["connection"].value;
        bool alive = (!strcmp("keep-alive", type.c_str())) || (!strcmp("Keep-Alive", type.c_str()));
        r->inInfo.connectionType = alive ? ConnectionType::KEEP_ALIVE : ConnectionType::CLOSED;
    }
    else if (r->httpVersion > 1000)
    {
        r->inInfo.connectionType = ConnectionType::KEEP_ALIVE;
    }
    else
    {
        r->inInfo.connectionType = ConnectionType::CLOSED;
    }

    return OK;
}

int processRequest(std::shared_ptr<Request> r)
{
    Connection *c = r->c;
    c->read_.handler = blockReading;

    // // register EPOLLOUT
    // cyclePtr->eventProccessor->modFd(c->fd_.getFd(), EPOLLIN | EPOLLOUT | EPOLLET, c);
    // c->write_.handler = runPhases;

    r->atPhase = 0;
    runPhases(&c->write_);
    return OK;
}

int processUpsStatusLine(std::shared_ptr<Request> upsr)
{
    std::shared_ptr<Upstream> ups = upsr->c->upstream_;

    int ret = parseStatusLine(upsr, &ups->ctx.status);
    if (ret != OK)
    {
        return ret;
    }

    ups->processHandler = processUpsHeaders;
    return processUpsHeaders(upsr);
}

int processUpsHeaders(std::shared_ptr<Request> upsr)
{
    std::shared_ptr<Upstream> ups = upsr->c->upstream_;
    int ret;

    while (1)
    {
        ret = parseHeaderLine(upsr, 1);
        if (ret == OK)
        {
            upsr->inInfo.headers.emplace_back(std::string(upsr->headerNameStart, upsr->headerNameEnd),
                                              std::string(upsr->headerValueStart, upsr->headerValueEnd));
            Header &now = upsr->inInfo.headers.back();
            upsr->inInfo.headerNameValueMap[toLower(now.name)] = now;
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

            ups->processHandler = processUpsBody;
            return processUpsBody(upsr);
        }
    }
}

int processUpsBody(std::shared_ptr<Request> upsr)
{
    int ret = 0;

    // no content-length && not chunked
    if (upsr->inInfo.contentLength == 0 && !upsr->inInfo.isChunked)
    {
        LOG_INFO << "Upstream process body done";
        return OK;
    }

    upsr->requestBody.rest = -1;

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
        upsr->c->read_.handler = blockReading;
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
    std::shared_ptr<Request> r = ev->c->request_;

    // OK: keep running phases
    // ERROR/DONE: quit phase running
    while ((size_t)r->atPhase < phases.size() && phases[r->atPhase].checker)
    {
        ret = phases[r->atPhase].checker(r, &phases[r->atPhase]);
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
    std::shared_ptr<Request> r = ev->c->request_;
    auto &buffer = ev->c->writeBuffer_;

    if (buffer.allRead() != 1)
    {
        int len = buffer.sendFd(ev->c->fd_.getFd(), &errno, 0);

        if (len < 0 && errno != EAGAIN)
        {
            LOG_WARN << "write response failed, errno: " << strerror(errno);
            finalizeRequest(r);
            return PHASE_ERR;
        }

        if (buffer.allRead() != 1)
        {
            r->c->write_.handler = writeResponse;
            cyclePtr->multiplexer->modFd(ev->c->fd_.getFd(), EVENTS(IN | OUT | ET), ev->c);
            return PHASE_QUIT;
        }
    }

    r->c->write_.handler = blockWriting;
    cyclePtr->multiplexer->modFd(r->c->fd_.getFd(), EVENTS(IN | ET), r->c);

    if (r->outInfo.restype == ResponseType::FILE)
    {
        sendfileEvent(&r->c->write_);
    }
    else if (r->outInfo.restype == ResponseType::STRING)
    {
        sendStrEvent(&r->c->write_);
    }
    else // RES_EMPTY
    {
        LOG_INFO << "RESPONSED";

        if (r->inInfo.connectionType == ConnectionType::KEEP_ALIVE)
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
    LOG_WARN << "Block reading triggered, FD:" << ev->c->fd_.getFd();
    return OK;
}

int blockWriting(Event *ev)
{
    LOG_WARN << "Block writing triggered, FD:" << ev->c->fd_.getFd();
    return OK;
}

// @return OK AGAIN ERROR
int processRequestBody(std::shared_ptr<Request> r)
{
    if (r->inInfo.isChunked)
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
    auto &buffer = r->c->readBuffer_;
    auto &rb = r->requestBody;

    if (rb.rest == -1)
    {
        rb.rest = r->inInfo.contentLength;
    }

    while (buffer.allRead() != 1)
    {
        int rlen = buffer.now->len - buffer.now->pos;
        if (rlen <= rb.rest)
        {
            rb.rest -= rlen;
            rb.listBody.emplace_back(buffer.now->start + buffer.now->pos, rlen);
            buffer.retrieve(rlen);
        }
        else
        {
            LOG_WARN << "ERROR";
            return ERROR;
        }

        if (buffer.now->pos == buffer.now->len)
        {
            if (buffer.now->next != NULL)
            {
                buffer.now = buffer.now->next;
            }
        }
    }

    if (rb.rest == 0)
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
    auto &buffer = r->c->readBuffer_;
    auto &rb = r->requestBody;

    if (rb.rest == -1)
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
            rb.rest = 0;
            return OK;
        }

        if (ret == AGAIN)
        {
            if (buffer.now->pos == buffer.now->len)
            {
                if (buffer.now->next != NULL)
                {
                    buffer.now = buffer.now->next;
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
    int preRead = 0;
    auto &buffer = r->c->readBuffer_;

    // no content-length && not chunked
    if (r->inInfo.contentLength == 0 && !r->inInfo.isChunked)
    {
        r->c->read_.handler = blockReading;
        if (postHandler)
        {
            LOG_INFO << "To post_handler";
            postHandler(r);
        }
        return OK;
    }

    r->requestBody.rest = -1;
    r->requestBody.postHandler = postHandler;

    preRead = buffer.now->pos;

    ret = processRequestBody(r);

    if (ret == ERROR)
    {
        LOG_WARN << "ERROR";
        return ret;
    }

    r->requestLength += preRead;

    r->c->read_.handler = readRequestBodyInner;
    cyclePtr->multiplexer->modFd(r->c->fd_.getFd(), EVENTS(IN | ET), r->c);

    r->c->write_.handler = blockWriting;

    ret = readRequestBodyInner(&r->c->read_);

    return ret;
}

// @return OK AGAIN ERROR
int readRequestBodyInner(Event *ev)
{
    Connection *c = ev->c;
    std::shared_ptr<Request> r = c->request_;

    auto &rb = r->requestBody;
    auto &buffer = c->readBuffer_;

    while (1)
    {
        if (rb.rest == 0)
        {
            break;
        }

        int ret = buffer.cRecvFd(c->fd_.getFd(), &errno, 0);

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
            r->requestLength += ret;
            ret = processRequestBody(r);

            if (ret == OK)
            {
                break;
            }

            return ret;
        }
    }

    r->c->read_.handler = blockReading;
    if (rb.postHandler)
    {
        LOG_INFO << "To post handler";
        rb.postHandler(r);
    }

    return OK;
}

int sendfileEvent(Event *ev)
{
    std::shared_ptr<Request> r = ev->c->request_;
    auto &filebody = r->outInfo.fileBody;

    ssize_t len =
        sendfile(r->c->fd_.getFd(), filebody.filefd.getFd(), &filebody.offset, filebody.fileSize - filebody.offset);

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

    if (filebody.fileSize - filebody.offset > 0)
    {
        r->c->write_.handler = sendfileEvent;
        cyclePtr->multiplexer->modFd(r->c->fd_.getFd(), EVENTS(IN | OUT | ET), r->c);
        return AGAIN;
    }

    cyclePtr->multiplexer->modFd(r->c->fd_.getFd(), EVENTS(IN | ET), r->c);
    r->c->write_.handler = blockWriting;
    filebody.filefd.closeFd();

    LOG_INFO << "SENDFILE RESPONSED";

    if (r->inInfo.connectionType == ConnectionType::KEEP_ALIVE)
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
    std::shared_ptr<Request> r = ev->c->request_;
    auto &strbody = r->outInfo.strBody;

    static int offset = 0;
    int bodysize = strbody.size();

    int len = send(r->c->fd_.getFd(), strbody.c_str() + offset, bodysize - offset, 0);

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
        r->c->write_.handler = sendStrEvent;
        cyclePtr->multiplexer->modFd(r->c->fd_.getFd(), EVENTS(IN | OUT | ET), r->c);
        return AGAIN;
    }

    offset = 0;
    cyclePtr->multiplexer->modFd(r->c->fd_.getFd(), EVENTS(IN | ET), r->c);
    r->c->write_.handler = blockWriting;

    LOG_INFO << "SENDSTR RESPONSED";

    if (r->inInfo.connectionType == ConnectionType::KEEP_ALIVE)
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
    int fd = r->c->fd_.getFd();

    Connection *c = r->c;

    r->init();

    r->c = c;

    c->read_.handler = waitRequestAgain;
    c->write_.handler = std::function<int(Event *)>();

    c->readBuffer_.init();
    c->writeBuffer_.init();

    cyclePtr->timer_.Add(c->fd_.getFd(), getTickMs() + 60000, setEventTimeout, (void *)&c->read_);

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

    c->quit = 1;

    // delete c at the end of a eventloop
    // cPool.recoverConnection(c);

    LOG_INFO << "set connection->quit, FD:" << fd;
    return 0;
}

int finalizeRequest(std::shared_ptr<Request> r)
{
    r->quit = 1;
    finalizeConnection(r->c);
    return 0;
}

std::string cacheControl(int fd)
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
    r->outInfo.resCode = code;
    r->outInfo.statusLine = getStatusLineByCode(code);
    r->outInfo.restype = ResponseType::STRING;
    auto &str = r->outInfo.strBody;
    str.append("<html>\n<head>\n\t<title>").append(hc.str).append("</title>\n</head>\n");
    str.append("<body>\n\t<center>\n\t\t<h1>")
        .append(hc.str)
        .append("</h1>\n\t</center>\n\t<hr>\n\t<center>MyServer</center>\n</body>\n</html>");

    r->outInfo.headers.emplace_back("Content-Type", getContentType("html", Charset::UTF_8));
    r->outInfo.headers.emplace_back("Content-Length", std::to_string(str.length()));
    r->outInfo.headers.emplace_back("Connection", "Keep-Alive");
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
    return "HTTP/1.1 " + hc.str + "\r\n";
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