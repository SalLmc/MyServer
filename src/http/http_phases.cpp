#include "../headers.h"

#include "../event/epoller.h"
#include "../utils/utils_declaration.h"
#include "http.h"
#include "http_phases.h"

#include "../memory/memory_manage.hpp"

extern std::unordered_map<std::string, std::string> extenContentTypeMap;
extern HeapMemory heap;
extern Cycle *cyclePtr;

u_char exten_save[16];

int testPhaseHandler(std::shared_ptr<Request> r)
{
    r->outInfo.resCode = ResponseCode::HTTP_OK;
    r->outInfo.statusLine = getStatusLineByCode(r->outInfo.resCode);
    r->outInfo.restype = ResponseType::STRING;

    r->outInfo.strBody.append("HELLO");

    r->outInfo.headers.emplace_back("Content-Type", getContentType("html", Charset::UTF_8));
    r->outInfo.headers.emplace_back("Content-Length", std::to_string(r->outInfo.strBody.length()));
    r->outInfo.headers.emplace_back("Connection", "Keep-Alive");

    return doResponse(r);
}

std::vector<PhaseHandler> phases{{genericPhaseChecker, {logPhaseHandler}},

                                 {genericPhaseChecker, {authAccessHandler, contentAccessHandler}},
                                 {genericPhaseChecker, {proxyPassHandler}},
                                 {genericPhaseChecker, {staticContentHandler, autoIndexHandler}},

                                 {genericPhaseChecker, {endPhaseHandler}}};

PhaseHandler::PhaseHandler(std::function<int(std::shared_ptr<Request>, PhaseHandler *)> checker,
                           std::vector<std::function<int(std::shared_ptr<Request>)>> &&handlers)
    : checker(checker), handlers(handlers)
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
            r->atPhase++;
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

int endPhaseHandler(std::shared_ptr<Request> r)
{
    LOG_CRIT << "endPhaseHandler PHASE_ERR";
    return PHASE_ERR;
}

int logPhaseHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Log handler, FD:" << r->c->fd_.getFd();
    char ipString[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &r->c->addr_.sin_addr, ipString, INET_ADDRSTRLEN);
    LOG_INFO << "ip: " << ipString;

    return PHASE_NEXT;
}

int authAccessHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Auth access handler, FD:" << r->c->fd_.getFd();
    auto &server = cyclePtr->servers_[r->c->serverIdx_];

    bool need_auth = 0;
    for (std::string path : server.auth_path)
    {
        if (isMatch(r->uri.toString(), path))
        {
            need_auth = 1;
            break;
        }
    }

    if (!need_auth)
    {
        return PHASE_CONTINUE;
    }

    int ok = 0;

    char authc[128] = {0};
    std::string path = "authcode_" + std::to_string(server.port);
    std::string auth;
    Fd fd(open(path.c_str(), O_RDONLY));
    if (fd.getFd() >= 0)
    {
        read(fd.getFd(), authc, sizeof(authc));
        auth.append(authc);
    }

    // std::string args = std::string(r->args.data, r->args.data + r->args.len);

    // if (args.find("code=" + auth) != std::string::npos)
    // {
    //     ok = 1;
    // }

    if (!ok)
    {
        if (r->inInfo.headerNameValueMap.count("code"))
        {
            if (r->inInfo.headerNameValueMap["code"].value == auth)
            {
                ok = 1;
            }
        }
    }

    if (ok)
    {
        return PHASE_CONTINUE;
    }

    setErrorResponse(r, ResponseCode::HTTP_UNAUTHORIZED);

    return doResponse(r);
}

int contentAccessHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Content access handler, FD:" << r->c->fd_.getFd();
    auto &server = cyclePtr->servers_[r->c->serverIdx_];
    std::string uri = std::string(r->uri.data, r->uri.data + r->uri.len);
    std::string path;
    Fd fd;

    // proxy_pass
    if (server.from != "")
    {
        if (uri.find(server.from) != std::string::npos)
        {
            r->nowProxyPass = 1;
            return PHASE_NEXT;
        }
    }

    // method check
    if (r->method != Method::GET)
    {
        setErrorResponse(r, ResponseCode::HTTP_FORBIDDEN);
        return doResponse(r);
    }

    path = server.root + uri;
    fd = open(path.c_str(), O_RDONLY);

    if (fd.getFd() < 0)
    {
        if (open(server.root.c_str(), O_RDONLY) < 0)
        {
            LOG_WARN << "can't open server root location";
            setErrorResponse(r, ResponseCode::HTTP_INTERNAL_SERVER_ERROR);
            return doResponse(r);
        }
        goto send404;
    }

    struct stat st;
    fstat(fd.getFd(), &st);
    if (st.st_mode & S_IFDIR)
    {
        // use index & try_files
        if (path.back() != '/')
        {
            path += "/";
        }

        // index
        std::string filePath = path + server.index;
        Fd filefd(open(filePath.c_str(), O_RDONLY));
        if (filefd.getFd() >= 0)
        {
            auto pos = filePath.find('.');
            if (pos != std::string::npos) // has exten
            {
                int extenlen = filePath.length() - pos - 1;
                for (int i = 0; i < extenlen; i++)
                {
                    exten_save[i] = filePath[i + pos + 1];
                }
                r->exten.data = exten_save;
                r->exten.len = extenlen;
            }

            fd.closeFd();
            fd.reset(std::move(filefd));

            goto fileok;
        }
        else
        {
            // try_files
            for (auto &name : server.try_files)
            {
                filePath = path + name;
                filefd = open(filePath.c_str(), O_RDONLY);
                if (filefd.getFd() >= 0)
                {
                    struct stat st;
                    fstat(fd.getFd(), &st);
                    if (!(st.st_mode & S_IFDIR))
                    {
                        auto pos = filePath.find('.');
                        if (pos != std::string::npos) // has exten
                        {
                            int extenlen = filePath.length() - pos - 1;
                            for (int i = 0; i < extenlen; i++)
                            {
                                exten_save[i] = filePath[i + pos + 1];
                            }
                            r->exten.data = exten_save;
                            r->exten.len = extenlen;
                        }

                        fd.reset(std::move(filefd));

                        goto fileok;
                    }
                    else
                    {
                        filefd.closeFd();
                    }
                }
            }
        }

        // try autoindex
        goto autoindex;
    }
    else
    {
        goto fileok;
    }

fileok:
    if (fd.getFd() >= 0)
    {
        if (r->inInfo.headerNameValueMap.count("if-none-match"))
        {
            std::string browser_etag = r->inInfo.headerNameValueMap["if-none-match"].value;
            if (matchEtag(fd.getFd(), browser_etag))
            {
                r->outInfo.resCode = ResponseCode::HTTP_NOT_MODIFIED;
                r->outInfo.statusLine = getStatusLineByCode(r->outInfo.resCode);
                r->outInfo.restype = ResponseType::EMPTY;
                r->outInfo.headers.emplace_back("Etag", std::move(browser_etag));
                return PHASE_NEXT;
            }
        }

        r->outInfo.resCode = ResponseCode::HTTP_OK;
        r->outInfo.statusLine = getStatusLineByCode(r->outInfo.resCode);
        r->outInfo.restype = ResponseType::FILE;

        struct stat st;
        fstat(fd.getFd(), &st);
        r->outInfo.fileBody.fileSize = st.st_size;

        // close fd after sendfile in writeResponse
        r->outInfo.fileBody.filefd.reset(std::move(fd));

        return PHASE_NEXT;
    }

autoindex:
    if (server.auto_index == 0)
    {
    send404:
        setErrorResponse(r, ResponseCode::HTTP_NOT_FOUND);
        return doResponse(r);
    }
    else
    {
        r->outInfo.resCode = ResponseCode::HTTP_OK;
        r->outInfo.statusLine =getStatusLineByCode(r->outInfo.resCode);
        r->outInfo.restype = ResponseType::AUTO_INDEX;
        return PHASE_NEXT;
    }
}

int proxyPassHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Proxy pass handler, FD:" << r->c->fd_.getFd();

    if (r->nowProxyPass != 1)
    {
        return PHASE_NEXT;
    }
    r->nowProxyPass = 0;

    LOG_INFO << "Need proxy pass";

    int ret = readRequestBody(r, initUpstream);
    LOG_INFO << "readRequestBody:" << ret;
    if (ret != OK)
    {
        LOG_WARN << "PHASE ERR";
        return PHASE_ERR;
    }

    return PHASE_QUIT;
}

int initUpstream(std::shared_ptr<Request> r)
{
    LOG_INFO << "Initing upstream";

    bool isDomain = 0;

    // setup upstream server
    auto &server = cyclePtr->servers_[r->c->serverIdx_];
    std::string addr = server.to;

    std::string ip;
    std::string domain;
    int port = 80;

    try
    {
        auto upstreamInfo = getServer(addr);
        if (isHostname(upstreamInfo.first))
        {
            ip = getIpByDomain(upstreamInfo.first);
            isDomain = 1;
            domain = upstreamInfo.first;
        }
        else
        {
            ip = upstreamInfo.first;
        }
        port = upstreamInfo.second;
    }
    catch (const std::exception &e)
    {
        LOG_WARN << e.what();
        return ERROR;
    }

    // replace uri
    std::string fullUri = std::string(r->uriStart, r->uriEnd);
    std::string newUri = getLeftUri(addr) + fullUri.replace(0, server.from.length(), "");

    LOG_INFO << "Upstream to " << server.to << " -> " << ip << ":" << port << newUri;

    // setup connection
    Connection *upc = cyclePtr->pool_->getNewConnection();

    if (upc == NULL)
    {
        LOG_WARN << "get connection failed";
        finalizeRequest(r);
        return ERROR;
    }

    upc->fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if (upc->fd_.getFd() < 0)
    {
        LOG_WARN << "open fd failed";
        finalizeConnection(upc);
        finalizeRequest(r);
        return ERROR;
    }

    upc->addr_.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &upc->addr_.sin_addr);
    upc->addr_.sin_port = htons(port);

    if (connect(upc->fd_.getFd(), (struct sockaddr *)&upc->addr_, sizeof(upc->addr_)) < 0)
    {
        LOG_WARN << "CONNECT ERR, FINALIZE CONNECTION, errro: " << strerror(errno);
        finalizeConnection(upc);
        finalizeRequest(r);
        return ERROR;
    }

    setNonblocking(upc->fd_.getFd());

    LOG_INFO << "Upstream connected";

    // setup upstream
    std::shared_ptr<Upstream> ups(new Upstream());
    r->c->upstream_ = ups;
    upc->upstream_ = ups;
    ups->client = r->c;
    ups->upstream = upc;

    // setup send content
    auto &wb = upc->writeBuffer_;
    wb.append(r->methodName.toString() + " " + newUri + " HTTP/1.1\r\n");
    auto &in = r->inInfo;

    // headers
    bool needRewriteHost = 0;
    for (auto &x : in.headers)
    {
        if (isDomain && (x.name == "Host" || x.name == "host"))
        {
            needRewriteHost = 1;
            continue;
        }
        std::string thisHeader = x.name + ": " + x.value + "\r\n";
        wb.append(thisHeader);
    }
    if (needRewriteHost)
    {
        wb.append("Host: ");
        wb.append(domain);
        wb.append("\r\n");
    }
    wb.append("\r\n");

    // body
    for (auto &x : r->requestBody.listBody)
    {
        wb.append(x.toString());
    }

    // send
    if (cyclePtr->multiplexer->addFd(upc->fd_.getFd(), EVENTS(IN | OUT | ET), upc) != 1)
    {
        LOG_CRIT << "epoller addfd failed, error:" << strerror(errno);
    }
    return send2Upstream(&upc->write_);
}

int recvFromUpstream(Event *upc_ev)
{
    int n;
    int ret;
    Connection *upc = upc_ev->c;
    std::shared_ptr<Upstream> ups = upc->upstream_;
    std::shared_ptr<Request> cr = ups->client->request_;
    std::shared_ptr<Request> upsr = ups->upstream->request_;

    while (1)
    {
        n = upc->readBuffer_.cRecvFd(upc->fd_.getFd(), &errno, 0);
        if (n < 0 && errno == EAGAIN)
        {
            return AGAIN;
        }
        else if (n < 0 && errno != EAGAIN)
        {
            LOG_WARN << "Recv error";
            finalizeRequest(upsr);
            finalizeRequest(cr);
            // heap.hDelete(upc->ups_);
            return ERROR;
        }
        else if (n == 0)
        {
            LOG_WARN << "Upstream server close connection";
            finalizeRequest(upsr);
            finalizeRequest(cr);
            // heap.hDelete(upc->ups_);
            return ERROR;
        }
        else
        {
            // processStatusLine->processHeaders->processBody
            ret = ups->processHandler(upsr);
            if (ret == AGAIN)
            {
                continue;
            }
            else if (ret == ERROR)
            {
                LOG_WARN << "Process error";
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
    for (auto &x : upsr->inInfo.headers)
    {
        upsr->c->writeBuffer_.append(x.name + ": " + x.value + "\r\n");
    }
    upsr->c->writeBuffer_.append("\r\n");
    for (auto &x : upsr->requestBody.listBody)
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

    return send2Client(&upc->write_);
}

int send2Upstream(Event *upc_ev)
{
    Connection *upc = upc_ev->c;
    std::shared_ptr<Upstream> ups = upc->upstream_;
    std::shared_ptr<Request> cr = upc->upstream_->client->request_;
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
                if (cyclePtr->multiplexer->modFd(upc->fd_.getFd(), EVENTS(IN | OUT | ET), upc))
                {
                    upc->write_.handler = send2Upstream;
                    return AGAIN;
                }
            }
            else
            {
                LOG_WARN << "SEND ERR, FINALIZE CONNECTION";
                finalizeConnection(upc);
                finalizeRequest(cr);
                // heap.hDelete(upc->ups_);
                return ERROR;
            }
        }
        else if (ret == 0)
        {
            LOG_WARN << "Upstream close connection, FINALIZE CONNECTION";
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

    // remove EPOLLOUT events
    if (cyclePtr->multiplexer->modFd(upc->fd_.getFd(), EVENTS(IN | ET), upc) != 1)
    {
        LOG_CRIT << "epoller modfd failed, error:" << strerror(errno);
    }

    upc->write_.handler = blockWriting;
    upc->read_.handler = recvFromUpstream;

    ups->processHandler = processUpsStatusLine;
    std::shared_ptr<Request> upsr(new Request());
    upsr->c = ups->upstream;
    ups->upstream->request_ = upsr;

    LOG_INFO << "Send client data to upstream complete";

    return recvFromUpstream(&upc->read_);
}

int staticContentHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Static content handler, FD:" << r->c->fd_.getFd();
    if (r->outInfo.restype == ResponseType::FILE)
    {
        LOG_INFO << "RES_FILE";
        r->outInfo.contentLength = r->outInfo.fileBody.fileSize;
        std::string etag = cacheControl(r->outInfo.fileBody.filefd.getFd());
        if (etag != "")
        {
            r->outInfo.headers.emplace_back("Etag", std::move(etag));
        }
    }
    else if (r->outInfo.restype == ResponseType::STRING)
    {
        LOG_INFO << "RES_STR";
        r->outInfo.contentLength = r->outInfo.strBody.length();
    }
    else if (r->outInfo.restype == ResponseType::EMPTY)
    {
        LOG_INFO << "RES_EMTPY";
        r->outInfo.contentLength = 0;
    }
    else if (r->outInfo.restype == ResponseType::AUTO_INDEX)
    {
        LOG_INFO << "RES_AUTO_INDEX";
        return PHASE_CONTINUE;
    }

    std::string exten = std::string(r->exten.data, r->exten.data + r->exten.len);

    r->outInfo.headers.emplace_back("Content-Type", getContentType(exten, Charset::UTF_8));
    r->outInfo.headers.emplace_back("Content-Length", std::to_string(r->outInfo.contentLength));
    r->outInfo.headers.emplace_back("Connection", "Keep-Alive");

    return doResponse(r);
}

int autoIndexHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Auto index handler, FD:" << r->c->fd_.getFd();
    if (r->outInfo.restype != ResponseType::AUTO_INDEX)
    {
        LOG_WARN << "Pass auto index handler";
        return PHASE_NEXT;
    }

    // setup
    r->outInfo.restype = ResponseType::STRING;

    exten_save[0] = 'h', exten_save[1] = 't', exten_save[2] = 'm', exten_save[3] = 'l', exten_save[4] = '\0';
    r->exten.data = exten_save;
    r->exten.len = 4;

    // auto index
    auto &out = r->outInfo;
    auto &server = cyclePtr->servers_[r->c->serverIdx_];

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
        setErrorResponse(r, ResponseCode::HTTP_INTERNAL_SERVER_ERROR);
        return doResponse(r);
    }

    directory.getInfos(root + subpath);

    out.strBody.append(title);
    out.strBody.append(header);
    out.strBody.append(subpath);
    out.strBody.append("</h1>" CRLF "<hr>" CRLF);
    out.strBody.append("<pre><a href=\"../\">../</a>" CRLF);

    for (auto &x : directory.infos)
    {
        out.strBody.append("<a href=\"");
        out.strBody.append(urlEncode(x.name, '/'));
        out.strBody.append("\">");
        out.strBody.append(x.name);
        out.strBody.append("</a>\t\t\t\t");
        out.strBody.append(mtime2Str(&x.mtime));
        out.strBody.append("\t");
        if (x.type == DT_DIR)
        {
            out.strBody.append("-");
        }
        else
        {
            out.strBody.append(byte2ProperStr(x.size_byte));
        }
        out.strBody.append(CRLF);
    }

    out.strBody.append(tail);

    // headers
    r->outInfo.headers.emplace_back("Content-Type", getContentType("html", Charset::UTF_8));
    r->outInfo.headers.emplace_back("Content-Length", std::to_string(out.strBody.length()));
    r->outInfo.headers.emplace_back("Connection", "Keep-Alive");

    return doResponse(r);
}

int appendResponseLine(std::shared_ptr<Request> r)
{
    auto &writebuffer = r->c->writeBuffer_;
    auto &out = r->outInfo;

    writebuffer.append(out.statusLine);

    return OK;
}
int appendResponseHeader(std::shared_ptr<Request> r)
{
    auto &writebuffer = r->c->writeBuffer_;
    auto &out = r->outInfo;

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
    auto &out = r->outInfo;

    if (out.restype == ResponseType::EMPTY)
    {
        return OK;
    }

    if (out.restype == ResponseType::FILE)
    {
        // use sendfile in writeResponse
    }
    else if (out.restype == ResponseType::STRING)
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

    // auto &buffer = r->c->writeBuffer_;
    // for (auto &x : buffer.nodes)
    // {
    //     if (x.len == 0)
    //         break;
    //     LOG_INFO << x.toString();
    // }

    return writeResponse(&r->c->write_);
}

int send2Client(Event *upc_ev)
{
    Connection *upc = upc_ev->c;
    std::shared_ptr<Upstream> ups = upc->upstream_;
    Connection *c = ups->client;
    std::shared_ptr<Request> cr = c->request_;
    std::shared_ptr<Request> upsr = upc->request_;
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
                if (cyclePtr->multiplexer->modFd(upc->fd_.getFd(), EVENTS(IN | OUT | ET), upc))
                {
                    upc->write_.handler = send2Client;
                    return AGAIN;
                }
            }
            else
            {
                // printf("%s\n", strerror(errno));
                // printf("%d\n", c->fd_.getFd());
                LOG_WARN << "SEND ERR, FINALIZE CONNECTION";
                finalizeRequest(upsr);
                finalizeRequest(cr);
                // heap.hDelete(upc->ups_);
                return ERROR;
            }
        }
        else if (ret == 0)
        {
            LOG_WARN << "Upstream close connection, FINALIZE CONNECTION";
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
    if (cr->inInfo.connectionType == ConnectionType::KEEP_ALIVE)
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