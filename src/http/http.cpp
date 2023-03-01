#include "http.h"
#include "../core/memory_manage.hpp"
#include "../event/epoller.h"
#include "../event/event.h"
#include "../global.h"
#include "../util/utils_declaration.h"
#include "http_parse.h"
#include "http_phases.h"
#include <string>
#include <sys/sendfile.h>
#include <unordered_map>

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
};

int initListen(Cycle *cycle, int port)
{
    Connection *listen = addListen(cycle, port);
    listen->server_idx_ = cycle->listening_.size();

    cycle->listening_.push_back(listen);

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
    assert(listenC->fd_.getFd() > 0);

    int reuse = 1;
    setsockopt(listenC->fd_.getFd(), SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    setnonblocking(listenC->fd_.getFd());

    assert(bind(listenC->fd_.getFd(), (sockaddr *)&listenC->addr_, sizeof(listenC->addr_)) == 0);

    assert(listen(listenC->fd_.getFd(), 5) == 0);

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
    Connection *newc = cPool.getNewConnection();
    assert(newc != NULL);

    sockaddr_in *addr = &newc->addr_;
    socklen_t len = sizeof(*addr);

    newc->fd_ = accept(ev->c->fd_.getFd(), (sockaddr *)addr, &len);

    assert(newc->fd_.getFd() >= 0);

    LOG_INFO << "NEW CONNECTION";

    newc->server_idx_ = ev->c->server_idx_;

    setnonblocking(newc->fd_.getFd());

    newc->read_.handler = waitRequest;

    epoller.addFd(newc->fd_.getFd(), EPOLLIN | EPOLLET, newc);

    cyclePtr->timer_.Add(newc->fd_.getFd(), getTickMs() + 5000, setEventTimeout, (void *)&newc->read_);
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
    int len = c->readBuffer_.recvFd(c->fd_.getFd(), &errno, 0);

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
            LOG_INFO << "Read error";
            finalizeConnection(c);
            return -1;
        }
        else
        {
            return 0;
        }
    }

    Request *r = heap.hNew<Request>();
    r->c = c;
    c->data = r;

    ev->handler = processRequestLine;
    processRequestLine(ev);
    return 0;
}

int processRequestLine(Event *ev)
{
    // LOG_INFO << "process request line";
    Connection *c = ev->c;
    Request *r = (Request *)c->data;

    int ret = AGAIN;
    for (;;)
    {
        if (ret == AGAIN)
        {
            int rett = readRequestHeader(r);
            LOG_INFO << "readRequestHeader:" << rett;
            if (rett == AGAIN || rett == ERROR)
            {
                break;
            }
        }

        ret = parseRequestLine(r);
        LOG_INFO << "parse request line end, ret:" << ret;
        if (ret == OK)
        {

            /* the request line has been parsed successfully */

            r->request_line.len = r->request_end - r->request_start;
            r->request_line.data = r->request_start;
            r->request_length = r->c->readBuffer_.peek() - (char *)r->request_start;

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
    }
    return OK;
}

int processRequestHeaders(Event *ev)
{
    int rc;
    Connection *c;
    Request *r;

    c = ev->c;
    r = (Request *)c->data;

    // LOG_INFO << "process request headers";

    rc = AGAIN;

    for (;;)
    {
        if (rc == AGAIN)
        {
            int ret = readRequestHeader(r);
            if (ret == AGAIN || ret == ERROR)
            {
                break;
            }
        }

        rc = parseHeaderLine(r, 1);

        if (rc == OK)
        {

            r->request_length += (u_char *)r->c->readBuffer_.peek() - r->header_name_start;

            if (r->invalid_header)
            {

                /* there was error while a header line parsing */
                LOG_CRIT << "client sent invalid header line";
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

            r->request_length += (u_char *)r->c->readBuffer_.peek() - r->header_name_start;

            r->http_state = HttpState::PROCESS_REQUEST_STATE;

            rc = processRequestHeader(r);
            if (rc != OK)
            {
                break;
            }

            // LOG_INFO<<"Content length:"<<r->headers_in.content_length;
            // LOG_INFO<<"Chunked:"<<r->headers_in.chunked;
            // LOG_INFO<<"Connection type:"<<r->headers_in.connection_type;

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

int readRequestHeader(Request *r)
{
    // LOG_INFO << "read request header";
    Connection *c = r->c;
    assert(c != NULL);

    int n = c->readBuffer_.beginWrite() - c->readBuffer_.peek();
    if (n > 0)
    {
        return n;
    }

    n = c->readBuffer_.recvFd(c->fd_.getFd(), &errno, 0);

    if (n < 0)
    {
        if (errno != EAGAIN)
        {
            LOG_INFO << "Read error";
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

int processRequestUri(Request *r)
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

int processRequestHeader(Request *r)
{
    auto &mp = r->headers_in.header_name_value_map;

    if (mp.count("Host") || mp.count("host"))
    {
    }
    else
    {
        LOG_INFO << "Client headers error";
        finalizeRequest(r);
        return ERROR;
    }

    if (mp.count("Content-Length"))
    {
        r->headers_in.content_length = atoi(mp["Content-Length"].value.c_str());
    }

    if (mp.count("Transfer-Encoding"))
    {
        r->headers_in.chunked = (0 == strcmp("chunked", mp["Transfer-Encoding"].value.c_str()));
    }

    if (mp.count("Connection"))
    {
        auto &type = mp["Connection"].value;
        if (strcmp("keep-alive", type.c_str()) == 0)
        {
            r->headers_in.connection_type = CONNECTION_KEEP_ALIVE;
        }
        else
        {
            r->headers_in.connection_type = CONNECTION_CLOSE;
        }
    }

    return OK;
}

int processRequest(Request *r)
{
    Connection *c = r->c;
    c->read_.handler = blockReading;

    // // haven't register EPOLLOUT, so runPhases won't be triggered
    // c->write_.handler = runPhases;

    r->at_phase = 0;
    runPhases(&c->write_);
    return OK;
}

int runPhases(Event *ev)
{
    int ret = 0;
    Request *r = (Request *)ev->c->data;
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
    Request *r = (Request *)ev->c->data;
    auto &buffer = ev->c->writeBuffer_;

    if (buffer.readableBytes() > 0)
    {
        int len = buffer.sendFd(ev->c->fd_.getFd(), &errno, 0);
        if (len < 0)
        {
            LOG_CRIT << "write response failed";
            return ERROR;
        }

        if (buffer.readableBytes() > 0)
        {
            epoller.modFd(ev->c->fd_.getFd(), EPOLLIN | EPOLLOUT | EPOLLET, ev->c);
            return AGAIN;
        }
    }

    if (r->headers_out.restype == RES_FILE)
    {
        auto &filebody = r->headers_out.file_body;
        sendfile(r->c->fd_.getFd(), filebody.filefd.getFd(), NULL, filebody.file_size);
    }

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

int blockReading(Event *ev)
{
    LOG_INFO << "block reading triggered";
    return OK;
}

int keepAliveRequest(Request *r)
{
    LOG_INFO << "KEEPALIVE CONNECTION";

    int at_phase = r->at_phase;
    Connection *c = r->c;

    r->init();

    r->c=c;
    r->at_phase=at_phase;
    
    c->read_.handler = waitRequest;
    c->write_.handler = NULL;

    return 0;
}

int finalizeConnection(Connection *c)
{
    LOG_INFO << "FINALIZE CONNECTION";
    epoller.delFd(c->fd_.getFd());
    cPool.recoverConnection(c);
    return 0;
}

int finalizeRequest(Request *r)
{
    r->quit = 1;
    finalizeConnection(r->c);
    return 0;
}