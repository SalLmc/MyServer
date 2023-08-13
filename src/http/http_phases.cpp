#include "../headers.h"

#include "../event/epoller.h"
#include "../util/utils_declaration.h"
#include "http.h"
#include "http_phases.h"

#include "../memory/memory_manage.hpp"

extern std::unordered_map<std::string, std::string> exten_content_type_map;
extern HeapMemory heap;
extern Epoller epoller;
extern Cycle *cyclePtr;

u_char exten_save[16];

int testPhaseHandler(std::shared_ptr<Request> r)
{
    r->headers_out.status = HTTP_OK;
    r->headers_out.status_line = "HTTP/1.1 200 OK\r\n";
    r->headers_out.restype = RES_EMPTY;

    r->headers_out.headers.emplace_back("Content-Type",
                                        std::string(exten_content_type_map["html"] + "; charset=utf-8"));
    r->headers_out.headers.emplace_back("Content-Length", "0");
    r->headers_out.headers.emplace_back("Connection", "Keep-Alive");

    return doResponse(r);
}

int endPhaseHandler(std::shared_ptr<Request> r)
{
    LOG_CRIT << "endPhaseHandler PHASE_ERR";
    assert(0);
    return PHASE_ERR;
}

std::vector<PhaseHandler> phases{{genericPhaseChecker, {passPhaseHandler}},

                                 {genericPhaseChecker, {contentAccessHandler}},
                                 {genericPhaseChecker, {proxyPassHandler, staticContentHandler, autoIndexHandler}},

                                 {genericPhaseChecker, {endPhaseHandler}}};

PhaseHandler::PhaseHandler(std::function<int(std::shared_ptr<Request>, PhaseHandler *)> checkerr,
                           std::vector<std::function<int(std::shared_ptr<Request>)>> &&handlerss)
    : checker(checkerr), handlers(handlerss)
{
}

int genericPhaseChecker(std::shared_ptr<Request> r, PhaseHandler *ph)
{
    int ret = 0;

    // PHASE_CONTINUE: call the next function in this phase
    // PHASE_NEXT: go to next phase & let runPhases keep running phases
    // PHASE_ERR: do nothing & return
    for (size_t i = 0; i < ph->handlers.size(); i++)
    {
        ret = ph->handlers[i](r);
        if (ret == PHASE_NEXT)
        {
            r->at_phase++;
            return OK;
        }
        if (ret == PHASE_ERR)
        {
            return ERROR;
        }
        if (ret == PHASE_CONTINUE)
        {
            continue;
        }
        if (ret == PHASE_QUIT)
        {
            return DONE;
        }
    }
    // nothing went wrong & no function asks to go to the next phase
    return DONE;
}

int passPhaseHandler(std::shared_ptr<Request> r)
{
    return PHASE_NEXT;
}

int contentAccessHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Content access handler";
    auto &server = cyclePtr->servers_[r->c->server_idx_];
    std::string uri = std::string(r->uri.data, r->uri.data + r->uri.len);
    std::string path;
    Fd fd;

    // proxy_pass
    if (server.proxy_pass.from != "")
    {
        std::string checkUri = uri + ((uri.back() == '/') ? "" : "/");
        if (checkUri.find(server.proxy_pass.from) != std::string::npos)
        {
            r->now_proxy_pass = 1;
            return PHASE_NEXT;
        }
    }

    // method check
    if (r->method != Method::GET)
    {
        r->headers_out.status = HTTP_FORBIDDEN;
        r->headers_out.status_line = "HTTP/1.1 403 FORBIDDEN\r\n";

        std::string _403_path = "/home/sallmc/VSCode/MyServer/static/403.html";
        Fd _403fd(open(_403_path.c_str(), O_RDONLY));
        if (_403fd.getFd() < 0)
        {
            r->headers_out.restype = RES_STR;
            auto &str = r->headers_out.str_body;
            str.append("<html>\n<head>\n\t<title>403 Forbidden</title>\n</head>\n");
            str.append("<body>\n\t<center>\n\t\t<h1>403 "
                       "Forbidden</h1>\n\t</center>\n\t<hr>\n\t<center>MyServer</center>\n</body>\n</html>");

            r->headers_out.headers.emplace_back("Content-Type",
                                                std::string(exten_content_type_map["html"] + "; charset=utf-8"));
            r->headers_out.headers.emplace_back("Content-Length", std::to_string(str.length()));
            r->headers_out.headers.emplace_back("Connection", "Keep-Alive");
        }
        else
        {
            r->headers_out.restype = RES_FILE;
            struct stat st;
            fstat(_403fd.getFd(), &st);
            r->headers_out.file_body.filefd = std::move(_403fd);
            r->headers_out.file_body.file_size = st.st_size;

            r->headers_out.headers.emplace_back("Content-Type",
                                                std::string(exten_content_type_map["html"] + "; charset=utf-8"));
            r->headers_out.headers.emplace_back("Content-Length", std::to_string(st.st_size));
            r->headers_out.headers.emplace_back("Connection", "Keep-Alive");
        }

        return doResponse(r);
    }

    if (uri == "/")
    {
        path = server.root + "/" + server.index;
        fd = open(path.c_str(), O_RDONLY);

        if (fd.getFd() >= 0) // return index if exist
        {
            exten_save[0] = 'h', exten_save[1] = 't', exten_save[2] = 'm', exten_save[3] = 'l', exten_save[4] = '\0';
            r->exten.data = exten_save;
            r->exten.len = 4;

            goto fileok;
        }
        else
        {
            goto autoindex;
        }
    }
    else
    {
        path = server.root + uri;
        fd = open(path.c_str(), O_RDONLY);

        if (fd.getFd() >= 0)
        {
            struct stat st;
            fstat(fd.getFd(), &st);
            if (st.st_mode & S_IFDIR)
            {
                goto autoindex;
            }
            else
            {
                goto fileok;
            }
        }
        else
        {
            // tryfiles...
            for (auto &name : server.try_files)
            {
                path = server.root + "/" + name;
                fd = open(path.c_str(), O_RDONLY);
                if (fd.getFd() >= 0)
                {
                    struct stat st;
                    fstat(fd.getFd(), &st);
                    if (!(st.st_mode & S_IFDIR))
                    {
                        auto pos = path.find('.');
                        if (pos != std::string::npos) // has exten
                        {
                            int extenlen = path.length() - pos - 1;
                            for (int i = 0; i < extenlen; i++)
                            {
                                exten_save[i] = path[i + pos + 1];
                            }
                            r->exten.data = exten_save;
                            r->exten.len = extenlen;
                        }
                        goto fileok;
                    }
                }
            }
            goto send404;
        }
    }

fileok:
    if (fd.getFd() >= 0)
    {
        if (r->headers_in.header_name_value_map.count("If-None-Match"))
        {
            std::string browser_etag = r->headers_in.header_name_value_map["If-None-Match"].value;
            if (matchEtag(fd.getFd(), browser_etag))
            {
                r->headers_out.status = HTTP_NOT_MODIFIED;
                r->headers_out.status_line = "HTTP/1.1 304 Not Modified\r\n";
                r->headers_out.restype = RES_EMPTY;
                r->headers_out.headers.emplace_back("Etag", std::move(browser_etag));
                return PHASE_NEXT;
            }
        }

        r->headers_out.status = HTTP_OK;
        r->headers_out.status_line = "HTTP/1.1 200 OK\r\n";
        r->headers_out.restype = RES_FILE;

        struct stat st;
        fstat(fd.getFd(), &st);
        r->headers_out.file_body.file_size = st.st_size;

        // close fd after sendfile in writeResponse
        r->headers_out.file_body.filefd = std::move(fd);

        return PHASE_NEXT;
    }

autoindex:
    if (server.auto_index == 0)
    {
    send404:
        r->headers_out.status = HTTP_NOT_FOUND;
        r->headers_out.status_line = "HTTP/1.1 404 NOT FOUND\r\n";

        std::string _404_path = "/home/sallmc/VSCode/MyServer/static/404.html";
        Fd _404fd(open(_404_path.c_str(), O_RDONLY));
        if (_404fd.getFd() < 0)
        {
            r->headers_out.restype = RES_STR;
            auto &str = r->headers_out.str_body;
            str.append("<html>\n<head>\n\t<title>404 Not Found</title>\n</head>\n");
            str.append("<body>\n\t<center>\n\t\t<h1>404 Not "
                       "Found</h1>\n\t</center>\n\t<hr>\n\t<center>MyServer</center>\n</body>\n</html>");

            r->headers_out.headers.emplace_back("Content-Type",
                                                std::string(exten_content_type_map["html"] + "; charset=utf-8"));
            r->headers_out.headers.emplace_back("Content-Length", std::to_string(str.length()));
            r->headers_out.headers.emplace_back("Connection", "Keep-Alive");
        }
        else
        {
            r->headers_out.restype = RES_FILE;
            struct stat st;
            fstat(_404fd.getFd(), &st);
            r->headers_out.file_body.filefd = std::move(_404fd);
            r->headers_out.file_body.file_size = st.st_size;

            r->headers_out.headers.emplace_back("Content-Type",
                                                std::string(exten_content_type_map["html"] + "; charset=utf-8"));
            r->headers_out.headers.emplace_back("Content-Length", std::to_string(st.st_size));
            r->headers_out.headers.emplace_back("Connection", "Keep-Alive");
        }

        return doResponse(r);
    }
    else
    {
        r->headers_out.status = HTTP_OK;
        r->headers_out.status_line = "HTTP/1.1 200 OK\r\n";
        r->headers_out.restype = RES_AUTO_INDEX;
        return PHASE_NEXT;
    }
}

int proxyPassHandler(std::shared_ptr<Request> r)
{
    if (r->now_proxy_pass != 1)
    {
        return PHASE_CONTINUE;
    }
    r->now_proxy_pass = 0;

    LOG_INFO << "Proxy pass handler";

    int ret = readRequestBody(r, initUpstream);
    LOG_INFO << "readRequestBody:" << ret;
    if (ret != OK)
    {
        LOG_INFO << "PHASE ERR";
        return PHASE_ERR;
    }

    return OK;
}

int initUpstream(std::shared_ptr<Request> r)
{
    LOG_INFO << "Initing upstream";

    // printf("%s\n", std::string(r->request_start, r->request_start + r->request_length).c_str());

    // setup upstream server
    auto &server = cyclePtr->servers_[r->c->server_idx_];
    std::string addr = server.proxy_pass.to;
    std::string ip = getIp(addr);
    int port = getPort(addr);

    // replace uri
    std::string fullUri = std::string(r->uri_start, r->uri_end);
    std::string newUri = ("/" + fullUri).replace(1, server.proxy_pass.from.length(), "");

    // setup connection
    Connection *upc = cyclePtr->pool_->getNewConnection();
    assert(upc != NULL);
    upc->fd_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(upc->fd_.getFd() >= 0);
    upc->addr_.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &upc->addr_.sin_addr);
    upc->addr_.sin_port = htons(port);
    if (connect(upc->fd_.getFd(), (struct sockaddr *)&upc->addr_, sizeof(upc->addr_)) < 0)
    {
        LOG_INFO << "CONNECT ERR, FINALIZE CONNECTION";
        finalizeConnection(upc);
        finalizeRequest(r);
        return ERROR;
    }
    setnonblocking(upc->fd_.getFd());

    LOG_INFO << "Upstream to: " << ip << ":" << port << newUri << " with FD:" << upc->fd_.getFd();

    // setup upstream
    std::shared_ptr<Upstream> ups(new Upstream());
    r->c->ups_ = ups;
    upc->ups_ = ups;
    ups->c4client = r->c;
    ups->c4upstream = upc;

    // setup send content
    auto &wb = upc->writeBuffer_;
    wb.append(r->method_name.toString() + " " + newUri + " HTTP/1.1\r\n");
    auto &in = r->headers_in;
    for (auto &x : in.headers)
    {
        std::string thisHeader = x.name + ": " + x.value + "\r\n";
        wb.append(thisHeader);
    }
    wb.append("\r\n");
    for (auto &x : r->request_body.lbody)
    {
        wb.append(x.toString());
    }
    // send
    return send2upstream(&upc->write_);
}

int upstreamRecv(Event *upc_ev)
{
    int n;
    int ret;
    Connection *upc = upc_ev->c;
    std::shared_ptr<Upstream> ups = upc->ups_;
    std::shared_ptr<Request> cr = ups->c4client->data_;
    std::shared_ptr<Request> upsr = ups->c4upstream->data_;

    while (1)
    {
        n = upc->readBuffer_.cRecvFd(upc->fd_.getFd(), &errno, 0);
        if (n < 0 && errno == EAGAIN)
        {
            return AGAIN;
        }
        else if (n < 0 && errno != EAGAIN)
        {
            LOG_INFO << "Recv error";
            finalizeRequest(upsr);
            finalizeRequest(cr);
            // heap.hDelete(upc->ups_);
            return ERROR;
        }
        else if (n == 0)
        {
            LOG_INFO << "Upstream server close connection";
            finalizeRequest(upsr);
            finalizeRequest(cr);
            // heap.hDelete(upc->ups_);
            return ERROR;
        }
        else
        {
            // processStatusLine->processHeaders->processBody
            ret = ups->process_handler(upsr);
            if (ret == AGAIN)
            {
                continue;
            }
            else if (ret == ERROR)
            {
                LOG_INFO << "Process error";
                finalizeRequest(upsr);
                finalizeRequest(cr);
                // heap.hDelete(upc->ups_);
                return ERROR;
            }
            break;
        }
    }

    LOG_INFO << "Upstream recv done";

    // for (auto &x : upsr->request_body.lbody)
    // {
    //     printf("%s", x.toString().c_str());
    // }
    // printf("\n");

    upsr->c->writeBuffer_.append("HTTP/1.1 " + std::string(ups->ctx.status.start, ups->ctx.status.end) + "\r\n");
    for (auto &x : upsr->headers_in.headers)
    {
        upsr->c->writeBuffer_.append(x.name + ": " + x.value + "\r\n");
    }
    upsr->c->writeBuffer_.append("\r\n");
    for (auto &x : upsr->request_body.lbody)
    {
        upsr->c->writeBuffer_.append(x.toString());
    }

    // auto now = upc->writeBuffer_.now;
    // for (; now; now = now->next)
    // {
    //     if (now->len == 0)
    //         break;
    //     printf("%s", std::string(now->start + now->pos, now->start + now->len).c_str());
    // }
    // printf("\n");

    return upsResponse2Client(&upc->write_);
}

int send2upstream(Event *upc_ev)
{
    Connection *upc = upc_ev->c;
    std::shared_ptr<Upstream> ups = upc->ups_;
    std::shared_ptr<Request> cr = upc->ups_->c4client->data_;
    int ret = 0;

    for (; upc->writeBuffer_.allRead() != 1;)
    {
        ret = upc->writeBuffer_.sendFd(upc->fd_.getFd(), &errno, 0);

        if (upc->writeBuffer_.allRead())
        {
            break;
        }

        if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                if (epoller.modFd(upc->fd_.getFd(), EPOLLIN | EPOLLOUT | EPOLLET, upc))
                {
                    upc->write_.handler = send2upstream;
                    return AGAIN;
                }
            }
            else
            {
                LOG_INFO << "SEND ERR, FINALIZE CONNECTION";
                finalizeConnection(upc);
                finalizeRequest(cr);
                // heap.hDelete(upc->ups_);
                return ERROR;
            }
        }
        else if (ret == 0)
        {
            LOG_INFO << "Upstream close connection, FINALIZE CONNECTION";
            finalizeConnection(upc);
            finalizeRequest(cr);
            // heap.hDelete(upc->ups_);
            return ERROR;
        }
        else
        {
            continue;
        }
    }

    epoller.modFd(upc->fd_.getFd(), EPOLLIN | EPOLLET, upc);
    upc->write_.handler = blockWriting;

    ups->process_handler = processStatusLine;
    std::shared_ptr<Request> upsr(new Request());
    upsr->c = ups->c4upstream;
    ups->c4upstream->data_ = upsr;

    LOG_INFO << "Send client data to upstream complete";

    // set epoller
    assert(epoller.addFd(upc->fd_.getFd(), EPOLLIN | EPOLLET, upc));
    upc->read_.handler = upstreamRecv;

    return upstreamRecv(&upc->read_);
}

int staticContentHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Static content handler";
    if (r->headers_out.restype == RES_FILE)
    {
        r->headers_out.content_length = r->headers_out.file_body.file_size;
        std::string etag = cacheControl(r->headers_out.file_body.filefd.getFd());
        if (etag != "")
        {
            r->headers_out.headers.emplace_back("Etag", std::move(etag));
        }
    }
    else if (r->headers_out.restype == RES_STR)
    {
        r->headers_out.content_length = r->headers_out.str_body.length();
    }
    else if (r->headers_out.restype == RES_EMPTY)
    {
        r->headers_out.content_length = 0;
    }
    else if (r->headers_out.restype == RES_AUTO_INDEX)
    {
        return PHASE_CONTINUE;
    }

    std::string exten = std::string(r->exten.data, r->exten.data + r->exten.len);
    if (exten_content_type_map.count(exten))
    {
        r->headers_out.headers.emplace_back("Content-Type",
                                            std::string(exten_content_type_map[exten]) + "; charset=utf-8");
    }
    else
    {
        r->headers_out.headers.emplace_back("Content-Type", "application/octet-stream");
    }
    r->headers_out.headers.emplace_back("Content-Length", std::to_string(r->headers_out.content_length));
    r->headers_out.headers.emplace_back("Connection", "Keep-Alive");

    return doResponse(r);
}

int autoIndexHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Auto index handler";
    if (r->headers_out.restype != RES_AUTO_INDEX)
    {
        LOG_WARN << "Pass auto index handler";
        return PHASE_NEXT;
    }

    // setup
    r->headers_out.restype = RES_STR;

    exten_save[0] = 'h', exten_save[1] = 't', exten_save[2] = 'm', exten_save[3] = 'l', exten_save[4] = '\0';
    r->exten.data = exten_save;
    r->exten.len = 4;

    // auto index
    auto &out = r->headers_out;
    auto &server = cyclePtr->servers_[r->c->server_idx_];

    static char title[] = "<html>" CRLF "<head><title>Index of ";
    static char header[] = "</title></head>" CRLF "<body>" CRLF "<h1>Index of ";
    static char tail[] = "</pre>" CRLF "<hr>" CRLF "</body>" CRLF "</html>";

    // open location
    auto &root = server.root;
    auto subpath = std::string(r->uri.data, r->uri.data + r->uri.len);
    Dir directory(opendir((root + subpath).c_str()));

    // can't open dir
    if (directory.dir == NULL)
    {
        LOG_WARN << "auto index directory open failed";
        out.status = HTTP_INTERNAL_SERVER_ERROR;
        out.status_line = "HTTP/1.1 500 Internal Server Error\r\n";
        out.str_body.append("<html>\r\n"
                            "<head>\r\n"
                            "\t<title>500 Internal Server Error</title>\r\n"
                            "</head>\r\n");

        out.str_body.append("</head>\r\n"
                            "<body>\r\n"
                            "</head>\r\n"
                            "<body>\r\n"
                            "\t<center>\r\n"
                            "\t\t<h1>500 Internal Server Error</h1>\r\n"
                            "\t</center>\r\n"
                            "\t<hr>\t\n"
                            "\t<center>MyServer</center>\r\n"
                            "</body>\r\n"
                            "</html>\r\n");
        goto resposne;
    }

    directory.getInfos(root + subpath);

    out.str_body.append(title);
    out.str_body.append(header);
    out.str_body.append(subpath);
    out.str_body.append("</h1>" CRLF "<hr>" CRLF);
    out.str_body.append("<pre><a href=\"../\">../</a>" CRLF);

    for (auto &x : directory.infos)
    {
        out.str_body.append("<a href=\"");
        out.str_body.append(UrlEncode(x.name, '/'));
        out.str_body.append("\">");
        out.str_body.append(x.name);
        out.str_body.append("</a>\t\t\t\t");
        out.str_body.append(mtime2str(&x.mtime));
        out.str_body.append("\t");
        if (x.type == DT_DIR)
        {
            out.str_body.append("-");
        }
        else
        {
            out.str_body.append(byte2properstr(x.size_byte));
        }
        out.str_body.append(CRLF);
    }

    out.str_body.append(tail);

resposne:
    // headers
    std::string exten = std::string(r->exten.data, r->exten.data + r->exten.len);
    if (exten_content_type_map.count(exten))
    {
        r->headers_out.headers.emplace_back("Content-Type",
                                            std::string(exten_content_type_map[exten] + "; charset=utf-8"));
    }
    else
    {
        r->headers_out.headers.emplace_back("Content-Type", "application/octet-stream");
    }
    r->headers_out.headers.emplace_back("Content-Length", std::to_string(out.str_body.length()));
    r->headers_out.headers.emplace_back("Connection", "Keep-Alive");

    return doResponse(r);
}

int appendResponseLine(std::shared_ptr<Request> r)
{
    auto &writebuffer = r->c->writeBuffer_;
    auto &out = r->headers_out;

    writebuffer.append(out.status_line);

    return OK;
}
int appendResponseHeader(std::shared_ptr<Request> r)
{
    auto &writebuffer = r->c->writeBuffer_;
    auto &out = r->headers_out;

    for (auto &x : out.headers)
    {
        std::string thisHeader = x.name + ": " + x.value + "\r\n";
        writebuffer.append(thisHeader);
    }

    writebuffer.append("\r\n");
    return OK;
}
int appendResponseBody(std::shared_ptr<Request> r)
{
    // auto &writebuffer = r->c->writeBuffer_;
    auto &out = r->headers_out;

    if (out.restype == RES_EMPTY)
    {
        return OK;
    }

    if (out.restype == RES_FILE)
    {
        // use sendfile in writeResponse
    }
    else if (out.restype == RES_STR)
    {
        // avoid copy, send str_body in writeResponse
        // writebuffer.append(out.str_body);
    }
    return OK;
}

std::list<std::function<int(std::shared_ptr<Request>)>> responseList{appendResponseLine, appendResponseHeader,
                                                                     appendResponseBody};

int doResponse(std::shared_ptr<Request> r)
{
    for (auto &x : responseList)
    {
        if (x(r) == ERROR)
        {
            return PHASE_ERR;
        }
    }

    return writeResponse(&r->c->write_);
}

int upsResponse2Client(Event *upc_ev)
{
    Connection *upc = upc_ev->c;
    std::shared_ptr<Upstream> ups = upc->ups_;
    Connection *c = ups->c4client;
    std::shared_ptr<Request> cr = c->data_;
    std::shared_ptr<Request> upsr = upc->data_;
    int ret = 0;

    LOG_INFO << "Write to client, FD:" << c->fd_.getFd();

    for (; upc->writeBuffer_.allRead() != 1;)
    {
        ret = upc->writeBuffer_.sendFd(c->fd_.getFd(), &errno, 0);

        if (upc->writeBuffer_.allRead())
        {
            break;
        }

        if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                if (epoller.modFd(upc->fd_.getFd(), EPOLLIN | EPOLLOUT | EPOLLET, upc))
                {
                    upc->write_.handler = upsResponse2Client;
                    return AGAIN;
                }
            }
            else
            {
                // printf("%s\n", strerror(errno));
                // printf("%d\n", c->fd_.getFd());
                LOG_INFO << "SEND ERR, FINALIZE CONNECTION";
                finalizeRequest(upsr);
                finalizeRequest(cr);
                // heap.hDelete(upc->ups_);
                return ERROR;
            }
        }
        else if (ret == 0)
        {
            LOG_INFO << "Upstream close connection, FINALIZE CONNECTION";
            finalizeRequest(upsr);
            finalizeRequest(cr);
            // heap.hDelete(upc->ups_);
            return ERROR;
        }
        else
        {
            continue;
        }
    }

    LOG_INFO << "PROXYPASS RESPONSED";

    finalizeRequest(upsr);
    if (cr->headers_in.connection_type == CONNECTION_KEEP_ALIVE)
    {
        keepAliveRequest(cr);
    }
    else
    {
        finalizeRequest(cr);
    }
    // heap.hDelete(ups);
    return OK;
}