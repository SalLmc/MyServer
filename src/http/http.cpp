#include "../headers.h"

#include "http.h"
#include "http_parse.h"
#include "http_phases.h"

#include "../event/epoller.h"
#include "../event/event.h"
#include "../global.h"
#include "../util/utils_declaration.h"

#include "../memory/memory_manage.hpp"

extern ConnectionPool cPool;
extern Epoller epoller;
extern Cycle *cyclePtr;
extern HeapMemory heap;
extern std::vector<PhaseHandler> phases;

std::unordered_map<std::string, std::string> exten_content_type_map = {
    {"html", "text/html"},
    {"htm", "text/html"},
    {"shtml", "text/html"},
    {"css", "text/css"},
    {"xml", "text/xml"},
    {"gif", "image/gif"},
    {"jpeg", "image/jpeg"},
    {"jpg", "image/jpeg"},
    {"js", "application/javascript"},
    {"atom", "application/atom+xml"},
    {"rss", "application/rss+xml"},
    {"mml", "text/mathml"},
    {"txt", "text/plain"},
    {"jad", "text/vnd.sun.j2me.app-descriptor"},
    {"wml", "text/vnd.wap.wml"},
    {"htc", "text/x-component"},
    {"avif", "image/avif"},
    {"png", "image/png"},
    {"svg", "image/svg+xml"},
    {"svgz", "image/svg+xml"},
    {"tif", "image/tiff"},
    {"tiff", "image/tiff"},
    {"wbmp", "image/vnd.wap.wbmp"},
    {"webp", "image/webp"},
    {"ico", "image/x-icon"},
    {"jng", "image/x-jng"},
    {"bmp", "image/x-ms-bmp"},
    {"woff", "font/woff"},
    {"woff2", "font/woff2"},
    {"jar", "application/java-archive"},
    {"war", "application/java-archive"},
    {"ear", "application/java-archive"},
    {"json", "application/json"},
    {"hqx", "application/mac-binhex40"},
    {"doc", "application/msword"},
    {"pdf", "application/pdf"},
    {"ps", "application/postscript"},
    {"eps", "application/postscript"},
    {"ai", "application/postscript"},
    {"rtf", "application/rtf"},
    {"m3u8", "application/vnd.apple.mpegurl"},
    {"kml", "application/vnd.google-earth.kml+xml"},
    {"kmz", "application/vnd.google-earth.kmz"},
    {"xls", "application/vnd.ms-excel"},
    {"eot", "application/vnd.ms-fontobject"},
    {"ppt", "application/vnd.ms-powerpoint"},
    {"odg", "application/vnd.oasis.opendocument.graphics"},
    {"odp", "application/vnd.oasis.opendocument.presentation"},
    {"ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {"odt", "application/vnd.oasis.opendocument.text"},
    {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {"wmlc", "application/vnd.wap.wmlc"},
    {"wasm", "application/wasm"},
    {"7z", "application/x-7z-compressed"},
    {"cco", "application/x-cocoa"},
    {"jardiff", "application/x-java-archive-diff"},
    {"jnlp", "application/x-java-jnlp-file"},
    {"run", "application/x-makeself"},
    {"pl", "application/x-perl"},
    {"pm", "application/x-perl"},
    {"prc", "application/x-pilot"},
    {"pdb", "application/x-pilot"},
    {"rar", "application/x-rar-compressed"},
    {"rpm", "application/x-redhat-package-manager"},
    {"sea", "application/x-sea"},
    {"swf", "application/x-shockwave-flash"},
    {"sit", "application/x-stuffit"},
    {"tcl", "application/x-tcl"},
    {"tk", "application/x-tcl"},
    {"der", "application/x-x509-ca-cert"},
    {"pem", "application/x-x509-ca-cert"},
    {"crt", "application/x-x509-ca-cert"},
    {"xpi", "application/x-xpinstall"},
    {"xhtml", "application/xhtml+xml"},
    {"xspf", "application/xspf+xml"},
    {"zip", "application/zip"},
    {"bin", "application/octet-stream"},
    {"exe", "application/octet-stream"},
    {"dll", "application/octet-stream"},
    {"deb", "application/octet-stream"},
    {"dmg", "application/octet-stream"},
    {"iso", "application/octet-stream"},
    {"img", "application/octet-stream"},
    {"msi", "application/octet-stream"},
    {"msp", "application/octet-stream"},
    {"msm", "application/octet-stream"},
    {"mid", "audio/midi"},
    {"midi", "audio/midi"},
    {"kar", "audio/midi"},
    {"mp3", "audio/mpeg"},
    {"ogg", "audio/ogg"},
    {"m4a", "audio/x-m4a"},
    {"ra", "audio/x-realaudio"},
    {"3gpp", "video/3gpp"},
    {"3gp", "video/3gpp"},
    {"ts", "video/mp2t"},
    {"mp4", "video/mp4"},
    {"mpeg", "video/mpeg"},
    {"mpg", "video/mpeg"},
    {"mov", "video/quicktime"},
    {"webm", "video/webm"},
    {"flv", "video/x-flv"},
    {"m4v", "video/x-m4v"},
    {"mng", "video/x-mng"},
    {"asx", "video/x-ms-asf"},
    {"asf", "video/x-ms-asf"},
    {"wmv", "video/x-ms-wmv"},
    {"avi", "video/x-msvideo"},
    {"md", "text/plain"},
};

int initListen(Cycle *cycle, int port)
{
    Connection *listen = addListen(cycle, port);
    listen->server_idx_ = cycle->listening_.size();

    cycle->listening_.push_back(listen);

    listen->read_.type = ACCEPT;
    listen->read_.handler = newConnection;

    return OK;
}

Connection *addListen(Cycle *cycle, int port)
{
    ConnectionPool *pool = cycle->pool_;
    Connection *listenC = pool->getNewConnection();

    assert(listenC != NULL);

    listenC->addr_.sin_addr.s_addr = INADDR_ANY;
    listenC->addr_.sin_family = AF_INET;
    listenC->addr_.sin_port = htons(port);

    listenC->fd_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenC->fd_.getFd() >= 0);

    int reuse = 1;
    setsockopt(listenC->fd_.getFd(), SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse));
    setnonblocking(listenC->fd_.getFd());

    assert(bind(listenC->fd_.getFd(), (sockaddr *)&listenC->addr_, sizeof(listenC->addr_)) == 0);

    assert(listen(listenC->fd_.getFd(), 4096) == 0);

    return listenC;
}

// static int recvPrint(Event *ev)
// {
//     int len = ev->c->readBuffer_.readFd(ev->c->fd_.getFd(), &errno);
//     if (len == 0)
//     {
//         printf("client close connection\n");
//         cPool.recoverConnection(ev->c);
//         return 1;
//     }
//     else if (len < 0 && errno != EAGAIN)
//     {
//         printf("errno:%s\n", strerror(errno));
//         cPool.recoverConnection(ev->c);
//         return 1;
//     }
//     printf("%d recv len:%d from client:%s\n", getpid(), len, ev->c->readBuffer_.allToStr().c_str());
//     ev->c->writeBuffer_.append(ev->c->readBuffer_.retrieveAllToStr());
//     epoller.modFd(ev->c->fd_.getFd(), EPOLLOUT | EPOLLET, ev->c);
//     return 0;
// }

// static int echoPrint(Event *ev)
// {
//     ev->c->writeBuffer_.writeFd(ev->c->fd_.getFd(), &errno);
//     ev->c->writeBuffer_.retrieveAll();

//     epoller.modFd(ev->c->fd_.getFd(), EPOLLIN | EPOLLET, ev->c);
//     return 0;
// }

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

        newc->server_idx_ = ev->c->server_idx_;

        setnonblocking(newc->fd_.getFd());

        newc->read_.handler = waitRequest;

        if (epoller.addFd(newc->fd_.getFd(), EPOLLIN | EPOLLET, newc) == 0)
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
        LOG_INFO << "Client timeout";
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
            LOG_INFO << "Read error: " << strerror(errno);
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
    c->data_ = r;

    ev->handler = processRequestLine;
    processRequestLine(ev);
    return 0;
}

int keepAlive(Event *ev)
{
    if (ev->timeout == TIMEOUT)
    {
        LOG_INFO << "Client timeout";
        finalizeRequest(ev->c->data_);
        return -1;
    }

    cyclePtr->timer_.Remove(ev->c->fd_.getFd());

    Connection *c = ev->c;

    LOG_INFO << "keepAlive recv from FD:" << c->fd_.getFd();

    int len = c->readBuffer_.cRecvFd(c->fd_.getFd(), &errno, 0);

    if (len == 0)
    {
        LOG_INFO << "Client close connection";
        finalizeRequest(ev->c->data_);
        return -1;
    }

    if (len < 0)
    {
        if (errno != EAGAIN)
        {
            LOG_INFO << "Read error: " << strerror(errno);
            finalizeRequest(ev->c->data_);
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
    std::shared_ptr<Request> r = c->data_;

    int ret = AGAIN;
    for (;;)
    {
        if (ret == AGAIN)
        {
            int rett = readRequestHeader(r);
            if (rett == ERROR)
            {
                LOG_INFO << "readRequestHeader ERROR";
                break;
            }
        }

        ret = parseRequestLine(r);
        if (ret == OK)
        {

            /* the request line has been parsed successfully */

            r->request_line.len = r->request_end - r->request_start;
            r->request_line.data = r->request_start;
            r->request_length = r->c->readBuffer_.now->start + r->c->readBuffer_.now->pos - r->request_start;

            LOG_INFO << "request line:"
                     << std::string(r->request_line.data, r->request_line.data + r->request_line.len);

            r->method_name.len = r->method_end - r->request_start + 1;
            r->method_name.data = r->request_line.data;

            if (r->http_protocol.data)
            {
                r->http_protocol.len = r->request_end - r->http_protocol.data;
            }

            if (processRequestUri(r) != OK)
            {
                break;
            }

            if (r->schema_end)
            {
                r->schema.len = r->schema_end - r->schema_start;
                r->schema.data = r->schema_start;
            }

            if (r->host_end)
            {
                r->host.len = r->host_end - r->host_start;
                r->host.data = r->host_start;
            }

            ev->handler = processRequestHeaders;
            processRequestHeaders(ev);

            break;
        }

        if (ret != AGAIN)
        {
            LOG_CRIT << "Parse request line failed";
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
    r = c->data_;

    // LOG_INFO << "process request headers";

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

            int ret = readRequestHeader(r);
            if (ret == ERROR || ret == AGAIN)
            {
                break;
            }
        }

        rc = parseHeaderLine(r, 1);

        if (rc == OK)
        {

            r->request_length += r->c->readBuffer_.now->start + r->c->readBuffer_.now->pos - r->header_name_start;

            if (r->invalid_header)
            {

                /* there was error while a header line parsing */
                LOG_CRIT << "Client sent invalid header line";
                continue;
            }

            /* a header line has been parsed successfully */

            r->headers_in.headers.emplace_back(std::string(r->header_name_start, r->header_name_end),
                                               std::string(r->header_start, r->header_end));

            Header &now = r->headers_in.headers.back();
            r->headers_in.header_name_value_map[now.name] = now;

            // LOG_INFO << "header:< " << now.name << ", " << now.value << " >";

            // // TODO
            // hh = ngx_hash_find(&cmcf->headers_in_hash, h->hash, h->lowcase_key, h->key.len);

            // if (hh && hh->handler(r, h, hh->offset) != NGX_OK)
            // {
            //     break;
            // }

            continue;
        }

        if (rc == PARSE_HEADER_DONE)
        {

            /* a whole header has been parsed successfully */

            LOG_INFO << "http header done";

            r->request_length += r->c->readBuffer_.now->start + r->c->readBuffer_.now->pos - r->header_name_start;

            r->http_state = HttpState::PROCESS_REQUEST_STATE;

            rc = processRequestHeader(r, 1);
            if (rc != OK)
            {
                break;
            }

            LOG_INFO << "Host: "
                     << (r->headers_in.header_name_value_map.count("Host")
                             ? r->headers_in.header_name_value_map["Host"].value
                             : r->headers_in.header_name_value_map["host"].value);

            LOG_INFO << "Port: " << cyclePtr->servers_[r->c->server_idx_].port;

            processRequest(r);

            break;
        }

        if (rc == AGAIN)
        {

            /* a header line parsing is still not complete */

            continue;
        }

        /* rc == NGX_HTTP_PARSE_INVALID_HEADER */

        LOG_WARN << "Client sent invalid header line";
        finalizeRequest(r);
        return ERROR;
        break;
    }

    return OK;
}

int readRequestHeader(std::shared_ptr<Request> r)
{
    // LOG_INFO << "read request header";
    Connection *c = r->c;
    assert(c != NULL);

    int n = c->readBuffer_.now->len - c->readBuffer_.now->pos;
    if (!c->readBuffer_.allRead())
    {
        return 1;
    }

    n = c->readBuffer_.cRecvFd(c->fd_.getFd(), &errno, 0);

    if (n < 0)
    {
        if (errno != EAGAIN)
        {
            LOG_INFO << "Read error: " << strerror(errno);
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

int processRequestUri(std::shared_ptr<Request> r)
{
    if (r->args_start)
    {
        r->uri.len = r->args_start - 1 - r->uri_start;
    }
    else
    {
        r->uri.len = r->uri_end - r->uri_start;
    }

    if (r->complex_uri || r->quoted_uri || r->empty_path_in_uri)
    {

        if (r->empty_path_in_uri)
        {
            r->uri.len++;
        }

        // free in Request::init()
        r->uri.data = (u_char *)heap.hMalloc(r->uri.len);

        if (parseComplexUri(r, 1) != OK)
        {
            r->uri.len = 0;
            LOG_CRIT << "Client sent invalid request";
            finalizeRequest(r);
            return ERROR;
        }
    }
    else
    {
        r->uri.data = r->uri_start;
    }

    r->unparsed_uri.len = r->uri_end - r->uri_start;
    r->unparsed_uri.data = r->uri_start;

    r->valid_unparsed_uri = r->empty_path_in_uri ? 0 : 1;

    if (r->uri_ext)
    {
        if (r->args_start)
        {
            r->exten.len = r->args_start - 1 - r->uri_ext;
        }
        else
        {
            r->exten.len = r->uri_end - r->uri_ext;
        }

        r->exten.data = r->uri_ext;
    }

    if (r->args_start && r->uri_end > r->args_start)
    {
        r->args.len = r->uri_end - r->args_start;
        r->args.data = r->args_start;
    }

    LOG_INFO << "http uri:" << std::string(r->uri.data, r->uri.data + r->uri.len);
    LOG_INFO << "http args:" << std::string(r->args.data, r->args.data + r->args.len);
    LOG_INFO << "http exten:" << std::string(r->exten.data, r->exten.data + r->exten.len);

    return OK;
}

int processRequestHeader(std::shared_ptr<Request> r, int need_host)
{
    auto &mp = r->headers_in.header_name_value_map;

    if (mp.count("Host") || mp.count("host"))
    {
    }
    else if (need_host)
    {
        LOG_INFO << "Client headers error";
        finalizeRequest(r);
        return ERROR;
    }

    if (mp.count("Content-Length"))
    {
        r->headers_in.content_length = atoi(mp["Content-Length"].value.c_str());
    }
    else
    {
        r->headers_in.content_length = 0;
    }

    if (mp.count("Transfer-Encoding"))
    {
        r->headers_in.chunked = (0 == strcmp("chunked", mp["Transfer-Encoding"].value.c_str()));
    }
    else
    {
        r->headers_in.chunked = 0;
    }

    if (mp.count("Connection"))
    {
        auto &type = mp["Connection"].value;
        bool alive = (!strcmp("keep-alive", type.c_str())) || (!strcmp("Keep-Alive", type.c_str()));
        r->headers_in.connection_type = alive ? CONNECTION_KEEP_ALIVE : CONNECTION_CLOSE;
    }
    else if (r->http_version > 1000)
    {
        r->headers_in.connection_type = CONNECTION_KEEP_ALIVE;
    }
    else
    {
        r->headers_in.connection_type = CONNECTION_CLOSE;
    }

    return OK;
}

int processRequest(std::shared_ptr<Request> r)
{
    Connection *c = r->c;
    c->read_.handler = blockReading;

    // register EPOLLOUT
    epoller.modFd(c->fd_.getFd(), EPOLLIN | EPOLLOUT | EPOLLET, c);
    c->write_.handler = runPhases;

    LOG_INFO << "Run phases";
    r->at_phase = 0;
    runPhases(&c->write_);
    return OK;
}

int processStatusLine(std::shared_ptr<Request> upsr)
{
    std::shared_ptr<Upstream> ups = upsr->c->ups_;

    int ret = parseStatusLine(upsr, &ups->ctx.status);
    if (ret != OK)
    {
        return ret;
    }

    ups->process_handler = processHeaders;
    return processHeaders(upsr);
}

int processHeaders(std::shared_ptr<Request> upsr)
{
    std::shared_ptr<Upstream> ups = upsr->c->ups_;
    int ret;

    while (1)
    {
        ret = parseHeaderLine(upsr, 1);
        if (ret == OK)
        {
            upsr->headers_in.headers.emplace_back(std::string(upsr->header_name_start, upsr->header_name_end),
                                                  std::string(upsr->header_start, upsr->header_end));
            Header &now = upsr->headers_in.headers.back();
            upsr->headers_in.header_name_value_map[now.name] = now;
            continue;
        }

        if (ret == AGAIN)
        {
            printf("AGAIN\n");
            return AGAIN;
        }

        if (ret == PARSE_HEADER_DONE)
        {
            LOG_INFO << "Upstream header done";
            processRequestHeader(upsr, 0);

            ups->process_handler = processBody;
            return processBody(upsr);
        }
    }
}

int processBody(std::shared_ptr<Request> upsr)
{
    int ret = 0;

    // no content-length && not chunked
    if (upsr->headers_in.content_length == 0 && !upsr->headers_in.chunked)
    {
        LOG_INFO << "Upstream process body done";
        return OK;
    }

    upsr->request_body.rest = -1;

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

    int ret = 0;
    std::shared_ptr<Request> r = ev->c->data_;

    // OK: keep running phases
    // AGAIN/ERROR: quit phase running
    while (r->at_phase < 11 && phases[r->at_phase].checker)
    {
        ret = phases[r->at_phase].checker(r, &phases[r->at_phase]);
        if (ret == OK)
        {
            continue;
        }
        else
        {
            return ret;
        }
    }
    return OK;
}

int writeResponse(Event *ev)
{
    std::shared_ptr<Request> r = ev->c->data_;
    auto &buffer = ev->c->writeBuffer_;

    if (buffer.allRead() != 1)
    {
        int len = buffer.sendFd(ev->c->fd_.getFd(), &errno, 0);

        if (len < 0 && errno != EAGAIN)
        {
            LOG_CRIT << "write response failed";
            finalizeRequest(r);
            return ERROR;
        }

        if (buffer.allRead() != 1)
        {
            r->c->write_.handler = writeResponse;
            epoller.modFd(ev->c->fd_.getFd(), EPOLLIN | EPOLLOUT | EPOLLET, ev->c);
            return AGAIN;
        }
    }

    r->c->write_.handler = blockWriting;
    epoller.modFd(r->c->fd_.getFd(), EPOLLIN | EPOLLET, r->c);

    if (r->headers_out.restype == RES_FILE)
    {
        sendfileEvent(&r->c->write_);
    }
    else if (r->headers_out.restype == RES_STR)
    {
        sendStrEvent(&r->c->write_);
    }
    else // RES_EMPTY
    {
        LOG_INFO << "RESPONSED";

        if (r->headers_in.connection_type == CONNECTION_KEEP_ALIVE)
        {
            keepAliveRequest(r);
        }
        else
        {
            finalizeRequest(r);
        }

        return OK;
    }
    return OK;
}

int blockReading(Event *ev)
{
    LOG_INFO << "Block reading triggered, FD:" << ev->c->fd_.getFd();
    return OK;
}

int blockWriting(Event *ev)
{
    LOG_INFO << "Block writing triggered, FD:" << ev->c->fd_.getFd();
    return OK;
}

// @return OK AGAIN ERROR
int processRequestBody(std::shared_ptr<Request> r)
{
    if (r->headers_in.chunked)
    {
        return requestBodyChunked(r);
    }
    else
    {
        return requestBodyLength(r);
    }
}

// @return OK AGAIN ERROR
int requestBodyLength(std::shared_ptr<Request> r)
{
    auto &buffer = r->c->readBuffer_;
    auto &rb = r->request_body;

    if (rb.rest == -1)
    {
        rb.rest = r->headers_in.content_length;
    }

    while (buffer.allRead() != 1)
    {
        int rlen = buffer.now->len - buffer.now->pos;
        if (rlen <= rb.rest)
        {
            rb.rest -= rlen;
            rb.lbody.emplace_back(buffer.now->start + buffer.now->pos, rlen);
            buffer.retrieve(rlen);
        }
        else
        {
            LOG_INFO << "ERROR";
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
int requestBodyChunked(std::shared_ptr<Request> r)
{
    auto &buffer = r->c->readBuffer_;
    auto &rb = r->request_body;

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
            LOG_INFO << "ERROR";
            return ERROR;
        }
    }

    LOG_INFO << "ERROR";
    return ERROR;
}

// @return OK AGAIN ERROR
int readRequestBody(std::shared_ptr<Request> r, std::function<int(std::shared_ptr<Request>)> post_handler)
{
    int ret = 0;
    int preRead = 0;
    auto &buffer = r->c->readBuffer_;

    // no content-length && not chunked
    if (r->headers_in.content_length == 0 && !r->headers_in.chunked)
    {
        r->c->read_.handler = blockReading;
        if (post_handler)
        {
            LOG_INFO << "To post_handler";
            post_handler(r);
        }
        return OK;
    }

    r->request_body.rest = -1;
    r->request_body.post_handler = post_handler;

    preRead = buffer.now->pos;

    ret = processRequestBody(r);

    if (ret == ERROR)
    {
        LOG_INFO << "ERROR";
        return ret;
    }

    r->request_length += preRead;

    r->c->read_.handler = readRequestBodyInner;
    epoller.modFd(r->c->fd_.getFd(), EPOLLIN | EPOLLET, r->c);

    r->c->write_.handler = blockWriting;

    ret = readRequestBodyInner(&r->c->read_);

    return ret;
}

// @return OK AGAIN ERROR
int readRequestBodyInner(Event *ev)
{
    Connection *c = ev->c;
    std::shared_ptr<Request> r = c->data_;

    auto &rb = r->request_body;
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
                LOG_INFO << "EAGAIN";
                return AGAIN;
            }
            else
            {
                finalizeRequest(r);
                LOG_INFO << "Recv body error";
                return ERROR;
            }
        }
        else
        {
            r->request_length += ret;
            ret = processRequestBody(r);

            if (ret == OK)
            {
                break;
            }

            return ret;
        }
    }

    r->c->read_.handler = blockReading;
    if (rb.post_handler)
    {
        LOG_INFO << "To post_handler";
        rb.post_handler(r);
    }

    return OK;
}

int sendfileEvent(Event *ev)
{
    std::shared_ptr<Request> r = ev->c->data_;
    auto &filebody = r->headers_out.file_body;

    int len =
        sendfile(r->c->fd_.getFd(), filebody.filefd.getFd(), &filebody.offset, filebody.file_size - filebody.offset);

    if (len < 0 && errno != EAGAIN)
    {
        LOG_INFO << "Sendfile error: " << strerror(errno);
        finalizeRequest(r);
        return ERROR;
    }
    else if (len == 0)
    {
        LOG_INFO << "Client close Connection";
        finalizeRequest(r);
        return ERROR;
    }

    if (filebody.file_size - filebody.offset > 0)
    {
        r->c->write_.handler = sendfileEvent;
        epoller.modFd(r->c->fd_.getFd(), EPOLLIN | EPOLLOUT | EPOLLET, r->c);
        return AGAIN;
    }

    epoller.modFd(r->c->fd_.getFd(), EPOLLIN | EPOLLET, r->c);
    r->c->write_.handler = blockWriting;
    filebody.filefd.closeFd();

    LOG_INFO << "SENDFILE RESPONSED";

    if (r->headers_in.connection_type == CONNECTION_KEEP_ALIVE)
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
    std::shared_ptr<Request> r = ev->c->data_;
    auto &strbody = r->headers_out.str_body;

    static int offset = 0;
    int bodysize = strbody.size();

    int len = send(r->c->fd_.getFd(), strbody.c_str() + offset, bodysize - offset, 0);

    if (len < 0 && errno != EAGAIN)
    {
        LOG_INFO << "Send error: " << strerror(errno);
        finalizeRequest(r);
        return ERROR;
    }
    else if (len == 0)
    {
        LOG_INFO << "Client close Connection";
        finalizeRequest(r);
        return ERROR;
    }

    offset += len;
    if (bodysize - offset > 0)
    {
        r->c->write_.handler = sendStrEvent;
        epoller.modFd(r->c->fd_.getFd(), EPOLLIN | EPOLLOUT | EPOLLET, r->c);
        return AGAIN;
    }

    offset = 0;
    epoller.modFd(r->c->fd_.getFd(), EPOLLIN | EPOLLET, r->c);
    r->c->write_.handler = blockWriting;

    LOG_INFO << "SENDSTR RESPONSED";

    if (r->headers_in.connection_type == CONNECTION_KEEP_ALIVE)
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

    int at_phase = r->at_phase;
    Connection *c = r->c;

    r->init();

    r->c = c;
    r->at_phase = at_phase;

    c->read_.handler = keepAlive;
    c->write_.handler = NULL;

    c->readBuffer_.init();
    c->writeBuffer_.init();

    cyclePtr->timer_.Add(c->fd_.getFd(), getTickMs() + 60000, setEventTimeout, (void *)&c->read_);

    LOG_INFO << "KEEPALIVE CONNECTION DONE, FD:" << fd;

    return 0;
}

int finalizeConnection(Connection *c)
{
    int fd = c->fd_.getFd();

    epoller.delFd(c->fd_.getFd());

    cPool.recoverConnection(c);

    LOG_INFO << "FINALIZE CONNECTION DONE, FD:" << fd;
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