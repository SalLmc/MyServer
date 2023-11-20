#include "../headers.h"

#include "../event/epoller.h"
#include "../utils/utils_declaration.h"
#include "http.h"
#include "http_phases.h"

#include "../memory/memory_manage.hpp"

extern std::unordered_map<std::string, std::string> extenContentTypeMap;
extern HeapMemory heap;
extern Server *serverPtr;

u_char extenSave[16];

int testPhaseHandler(std::shared_ptr<Request> r)
{
    r->outInfo_.resCode_ = ResponseCode::HTTP_OK;
    r->outInfo_.statusLine_ = getStatusLineByCode(r->outInfo_.resCode_);
    r->outInfo_.resType_ = ResponseType::STRING;

    r->outInfo_.strBody_.append("HELLO");

    r->outInfo_.headers_.emplace_back("Content-Type", getContentType("html", Charset::UTF_8));
    r->outInfo_.headers_.emplace_back("Content-Length", std::to_string(r->outInfo_.strBody_.length()));
    r->outInfo_.headers_.emplace_back("Connection", "Keep-Alive");

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
            r->atPhase_++;
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
    LOG_INFO << "Log handler, FD:" << r->c_->fd_.getFd();
    char ipString[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &r->c_->addr_.sin_addr, ipString, INET_ADDRSTRLEN);
    LOG_INFO << "ip: " << ipString;

    return PHASE_NEXT;
}

int authAccessHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Auth access handler, FD:" << r->c_->fd_.getFd();
    auto &server = serverPtr->servers_[r->c_->serverIdx_];

    bool need_auth = 0;
    for (std::string path : server.authPaths_)
    {
        if (isMatch(r->uri_.toString(), path))
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
    std::string path = "authcode_" + std::to_string(server.port_);
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
        if (r->inInfo_.headerNameValueMap_.count("code"))
        {
            if (r->inInfo_.headerNameValueMap_["code"].value_ == auth)
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
    LOG_INFO << "Content access handler, FD:" << r->c_->fd_.getFd();
    auto &server = serverPtr->servers_[r->c_->serverIdx_];
    std::string uri = std::string(r->uri_.data_, r->uri_.data_ + r->uri_.len_);
    std::string path;
    Fd fd;

    // proxy_pass
    if (server.from_ != "")
    {
        if (uri.find(server.from_) != std::string::npos)
        {
            r->nowProxyPass_ = 1;
            return PHASE_NEXT;
        }
    }

    // method check
    if (r->method_ != Method::GET)
    {
        setErrorResponse(r, ResponseCode::HTTP_FORBIDDEN);
        return doResponse(r);
    }

    path = server.root_ + uri;
    fd = open(path.c_str(), O_RDONLY);

    if (fd.getFd() < 0)
    {
        if (open(server.root_.c_str(), O_RDONLY) < 0)
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
        std::string filePath = path + server.index_;
        Fd filefd(open(filePath.c_str(), O_RDONLY));
        if (filefd.getFd() >= 0)
        {
            auto pos = filePath.find('.');
            if (pos != std::string::npos) // has exten
            {
                int extenlen = filePath.length() - pos - 1;
                for (int i = 0; i < extenlen; i++)
                {
                    extenSave[i] = filePath[i + pos + 1];
                }
                r->exten_.data_ = extenSave;
                r->exten_.len_ = extenlen;
            }

            fd.closeFd();
            fd.reset(std::move(filefd));

            goto fileok;
        }
        else // try_files
        {
            for (auto &name : server.tryFiles_)
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
                                extenSave[i] = filePath[i + pos + 1];
                            }
                            r->exten_.data_ = extenSave;
                            r->exten_.len_ = extenlen;
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
        if (r->inInfo_.headerNameValueMap_.count("if-none-match"))
        {
            std::string browser_etag = r->inInfo_.headerNameValueMap_["if-none-match"].value_;
            if (matchEtag(fd.getFd(), browser_etag))
            {
                r->outInfo_.resCode_ = ResponseCode::HTTP_NOT_MODIFIED;
                r->outInfo_.statusLine_ = getStatusLineByCode(r->outInfo_.resCode_);
                r->outInfo_.resType_ = ResponseType::EMPTY;
                r->outInfo_.headers_.emplace_back("Etag", std::move(browser_etag));
                return PHASE_NEXT;
            }
        }

        r->outInfo_.resCode_ = ResponseCode::HTTP_OK;
        r->outInfo_.statusLine_ = getStatusLineByCode(r->outInfo_.resCode_);
        r->outInfo_.resType_ = ResponseType::FILE;

        struct stat st;
        fstat(fd.getFd(), &st);
        r->outInfo_.fileBody_.fileSize_ = st.st_size;

        // close fd after sendfile in writeResponse
        r->outInfo_.fileBody_.filefd_.reset(std::move(fd));

        return PHASE_NEXT;
    }

autoindex:
    if (server.auto_index_ == 0)
    {
    send404:
        setErrorResponse(r, ResponseCode::HTTP_NOT_FOUND);
        return doResponse(r);
    }
    else
    {
        // access a directory but uri doesn't end with '/'
        if (uri.back() != '/')
        {
            setErrorResponse(r, ResponseCode::HTTP_MOVED_PERMANENTLY);
            r->outInfo_.headers_.emplace_back("Location", uri + "/");
            return doResponse(r);
        }

        r->outInfo_.resCode_ = ResponseCode::HTTP_OK;
        r->outInfo_.statusLine_ = getStatusLineByCode(r->outInfo_.resCode_);
        r->outInfo_.resType_ = ResponseType::AUTO_INDEX;
        return PHASE_NEXT;
    }
}

int proxyPassHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Proxy pass handler, FD:" << r->c_->fd_.getFd();

    if (r->nowProxyPass_ != 1)
    {
        return PHASE_NEXT;
    }
    r->nowProxyPass_ = 0;

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
    auto &server = serverPtr->servers_[r->c_->serverIdx_];
    std::string addr = server.to_;

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
    std::string fullUri = std::string(r->uriStart_, r->uriEnd_);
    std::string newUri = getLeftUri(addr) + fullUri.replace(0, server.from_.length(), "");

    LOG_INFO << "Upstream to " << server.to_ << " -> " << ip << ":" << port << newUri;

    // setup connection
    Connection *upc = serverPtr->pool_->getNewConnection();

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
    r->c_->upstream_ = ups;
    upc->upstream_ = ups;
    ups->client_ = r->c_;
    ups->upstream_ = upc;

    // setup send content
    auto &wb = upc->writeBuffer_;
    wb.append(r->methodName_.toString() + " " + newUri + " HTTP/1.1\r\n");
    auto &in = r->inInfo_;

    // headers
    bool needRewriteHost = 0;
    for (auto &x : in.headers_)
    {
        if (isDomain && (x.name_ == "Host" || x.name_ == "host"))
        {
            needRewriteHost = 1;
            continue;
        }
        std::string thisHeader = x.name_ + ": " + x.value_ + "\r\n";
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
    for (auto &x : r->requestBody_.listBody_)
    {
        wb.append(x.toString());
    }

    // send
    if (serverPtr->multiplexer_->addFd(upc->fd_.getFd(), EVENTS(IN | OUT | ET), upc) != 1)
    {
        LOG_CRIT << "epoller addfd failed, error:" << strerror(errno);
    }
    return send2Upstream(&upc->write_);
}

int recvFromUpstream(Event *upcEv)
{
    int n;
    int ret;
    Connection *upc = upcEv->c_;
    std::shared_ptr<Upstream> ups = upc->upstream_;
    std::shared_ptr<Request> cr = ups->client_->request_;
    std::shared_ptr<Request> upsr = ups->upstream_->request_;

    while (1)
    {
        n = upc->readBuffer_.recvFd(upc->fd_.getFd(), 0);
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
            ret = ups->processHandler_(upsr);
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

    upsr->c_->writeBuffer_.append("HTTP/1.1 " + std::string(ups->ctx_.status_.start_, ups->ctx_.status_.end_) + "\r\n");
    for (auto &x : upsr->inInfo_.headers_)
    {
        upsr->c_->writeBuffer_.append(x.name_ + ": " + x.value_ + "\r\n");
    }
    upsr->c_->writeBuffer_.append("\r\n");
    for (auto &x : upsr->requestBody_.listBody_)
    {
        upsr->c_->writeBuffer_.append(x.toString());
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

int send2Upstream(Event *upcEv)
{
    Connection *upc = upcEv->c_;
    std::shared_ptr<Upstream> ups = upc->upstream_;
    std::shared_ptr<Request> cr = upc->upstream_->client_->request_;
    int ret = 0;

    for (; upc->writeBuffer_.allRead() != 1;)
    {
        ret = upc->writeBuffer_.sendFd(upc->fd_.getFd(), 0);

        if (upc->writeBuffer_.allRead())
        {
            break;
        }

        if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                if (serverPtr->multiplexer_->modFd(upc->fd_.getFd(), EVENTS(IN | OUT | ET), upc))
                {
                    upc->write_.handler_ = send2Upstream;
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
    if (serverPtr->multiplexer_->modFd(upc->fd_.getFd(), EVENTS(IN | ET), upc) != 1)
    {
        LOG_CRIT << "epoller modfd failed, error:" << strerror(errno);
    }

    upc->write_.handler_ = blockWriting;
    upc->read_.handler_ = recvFromUpstream;

    ups->processHandler_ = processUpsStatusLine;
    std::shared_ptr<Request> upsr(new Request());
    upsr->c_ = ups->upstream_;
    ups->upstream_->request_ = upsr;

    LOG_INFO << "Send client data to upstream complete";

    return recvFromUpstream(&upc->read_);
}

int staticContentHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Static content handler, FD:" << r->c_->fd_.getFd();
    if (r->outInfo_.resType_ == ResponseType::FILE)
    {
        LOG_INFO << "RES_FILE";
        r->outInfo_.contentLength_ = r->outInfo_.fileBody_.fileSize_;
        std::string etag = getEtag(r->outInfo_.fileBody_.filefd_.getFd());
        if (etag != "")
        {
            r->outInfo_.headers_.emplace_back("Etag", std::move(etag));
        }
    }
    else if (r->outInfo_.resType_ == ResponseType::STRING)
    {
        LOG_INFO << "RES_STR";
        r->outInfo_.contentLength_ = r->outInfo_.strBody_.length();
    }
    else if (r->outInfo_.resType_ == ResponseType::EMPTY)
    {
        LOG_INFO << "RES_EMTPY";
        r->outInfo_.contentLength_ = 0;
    }
    else if (r->outInfo_.resType_ == ResponseType::AUTO_INDEX)
    {
        LOG_INFO << "RES_AUTO_INDEX";
        return PHASE_CONTINUE;
    }

    std::string exten = std::string(r->exten_.data_, r->exten_.data_ + r->exten_.len_);

    r->outInfo_.headers_.emplace_back("Content-Type", getContentType(exten, Charset::UTF_8));
    r->outInfo_.headers_.emplace_back("Content-Length", std::to_string(r->outInfo_.contentLength_));
    r->outInfo_.headers_.emplace_back("Connection", "Keep-Alive");

    return doResponse(r);
}

int autoIndexHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Auto index handler, FD:" << r->c_->fd_.getFd();
    if (r->outInfo_.resType_ != ResponseType::AUTO_INDEX)
    {
        LOG_WARN << "Pass auto index handler";
        return PHASE_NEXT;
    }

    // setup
    r->outInfo_.resType_ = ResponseType::STRING;

    extenSave[0] = 'h', extenSave[1] = 't', extenSave[2] = 'm', extenSave[3] = 'l', extenSave[4] = '\0';
    r->exten_.data_ = extenSave;
    r->exten_.len_ = 4;

    // auto index
    auto &out = r->outInfo_;
    auto &server = serverPtr->servers_[r->c_->serverIdx_];

    static char title[] = "<html>" CRLF "<head><title>Index of ";
    static char header[] = "</title></head>" CRLF "<body>" CRLF "<h1>Index of ";
    static char tail[] = "</pre>" CRLF "<hr>" CRLF "</body>" CRLF "</html>";

    // open location
    auto &root = server.root_;
    auto subpath = std::string(r->uri_.data_, r->uri_.data_ + r->uri_.len_);
    Dir directory(opendir((root + subpath).c_str()));

    // can't open dir
    if (directory.dir_ == NULL)
    {
        setErrorResponse(r, ResponseCode::HTTP_INTERNAL_SERVER_ERROR);
        return doResponse(r);
    }

    directory.getInfos(root + subpath);

    out.strBody_.append(title);
    out.strBody_.append(header);
    out.strBody_.append(subpath);
    out.strBody_.append("</h1>" CRLF "<hr>" CRLF);
    out.strBody_.append("<pre><a href=\"../\">../</a>" CRLF);

    for (auto &x : directory.infos_)
    {
        out.strBody_.append("<a href=\"");
        out.strBody_.append(urlEncode(x.name_, '/'));
        out.strBody_.append("\">");
        out.strBody_.append(x.name_);
        out.strBody_.append("</a>\t\t\t\t");
        out.strBody_.append(mtime2Str(&x.mtime_));
        out.strBody_.append("\t");
        if (x.type_ == DT_DIR)
        {
            out.strBody_.append("-");
        }
        else
        {
            out.strBody_.append(byte2ProperStr(x.sizeBytes_));
        }
        out.strBody_.append(CRLF);
    }

    out.strBody_.append(tail);

    // headers
    r->outInfo_.headers_.emplace_back("Content-Type", getContentType("html", Charset::UTF_8));
    r->outInfo_.headers_.emplace_back("Content-Length", std::to_string(out.strBody_.length()));
    r->outInfo_.headers_.emplace_back("Connection", "Keep-Alive");

    return doResponse(r);
}

int appendResponseLine(std::shared_ptr<Request> r)
{
    auto &writebuffer = r->c_->writeBuffer_;
    auto &out = r->outInfo_;

    writebuffer.append(out.statusLine_);

    return OK;
}
int appendResponseHeader(std::shared_ptr<Request> r)
{
    auto &writebuffer = r->c_->writeBuffer_;
    auto &out = r->outInfo_;

    for (auto &x : out.headers_)
    {
        std::string thisHeader = x.name_ + ": " + x.value_ + "\r\n";
        writebuffer.append(thisHeader);
    }

    writebuffer.append("\r\n");
    return OK;
}
int appendResponseBody(std::shared_ptr<Request> r)
{
    // auto &writebuffer = r->c->writeBuffer_;
    auto &out = r->outInfo_;

    if (out.resType_ == ResponseType::EMPTY)
    {
        return OK;
    }

    if (out.resType_ == ResponseType::FILE)
    {
        // use sendfile in writeResponse
    }
    else if (out.resType_ == ResponseType::STRING)
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

    return writeResponse(&r->c_->write_);
}

int send2Client(Event *upcEv)
{
    Connection *upc = upcEv->c_;
    std::shared_ptr<Upstream> ups = upc->upstream_;
    Connection *c = ups->client_;
    std::shared_ptr<Request> cr = c->request_;
    std::shared_ptr<Request> upsr = upc->request_;
    int ret = 0;

    LOG_INFO << "Write to client, FD:" << c->fd_.getFd();

    for (; upc->writeBuffer_.allRead() != 1;)
    {
        ret = upc->writeBuffer_.sendFd(c->fd_.getFd(), 0);

        if (upc->writeBuffer_.allRead())
        {
            break;
        }

        if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                if (serverPtr->multiplexer_->modFd(upc->fd_.getFd(), EVENTS(IN | OUT | ET), upc))
                {
                    upc->write_.handler_ = send2Client;
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
    if (cr->inInfo_.connectionType_ == ConnectionType::KEEP_ALIVE)
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