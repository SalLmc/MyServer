#include "../headers.h"

#include "../core/core.h"
#include "../event/event.h"
#include "../log/logger.h"
#include "../utils/utils.h"
#include "http.h"
#include "http_phases.h"

extern Server *serverPtr;

u_char extenSave[16];

int testPhaseHandler(std::shared_ptr<Request> r)
{
    r->contextOut_.resCode_ = HTTP_OK;
    r->contextOut_.statusLine_ = getStatusLineByCode(r->contextOut_.resCode_);
    r->contextOut_.resType_ = ResponseType::STRING;

    r->contextOut_.strBody_.append("HELLO");

    r->contextOut_.headers_.emplace_back("Content-Type", getContentType("html", Charset::UTF_8));
    r->contextOut_.headers_.emplace_back("Content-Length", std::to_string(r->contextOut_.strBody_.length()));
    r->contextOut_.headers_.emplace_back("Connection", "Keep-Alive");

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
        if (r->contextIn_.headerNameValueMap_.count("code"))
        {
            if (r->contextIn_.headerNameValueMap_["code"].value_ == auth)
            {
                ok = 1;
            }
        }
    }

    if (ok)
    {
        return PHASE_CONTINUE;
    }

    setErrorResponse(r, HTTP_UNAUTHORIZED);

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
    if (!server.from_.empty() && !server.to_.empty())
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
        setErrorResponse(r, HTTP_FORBIDDEN);
        return doResponse(r);
    }

    path = server.root_ + uri;
    fd = open(path.c_str(), O_RDONLY);

    if (fd.getFd() < 0)
    {
        if (open(server.root_.c_str(), O_RDONLY) < 0)
        {
            LOG_WARN << "can't open server root location";
            setErrorResponse(r, HTTP_INTERNAL_SERVER_ERROR);
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
        if (r->contextIn_.headerNameValueMap_.count("if-none-match"))
        {
            std::string browser_etag = r->contextIn_.headerNameValueMap_["if-none-match"].value_;
            if (etagMatched(fd.getFd(), browser_etag))
            {
                r->contextOut_.resCode_ = HTTP_NOT_MODIFIED;
                r->contextOut_.statusLine_ = getStatusLineByCode(r->contextOut_.resCode_);
                r->contextOut_.resType_ = ResponseType::EMPTY;
                r->contextOut_.headers_.emplace_back("Etag", std::move(browser_etag));
                return PHASE_NEXT;
            }
        }

        r->contextOut_.resCode_ = HTTP_OK;
        r->contextOut_.statusLine_ = getStatusLineByCode(r->contextOut_.resCode_);
        r->contextOut_.resType_ = ResponseType::FILE;

        struct stat st;
        fstat(fd.getFd(), &st);
        r->contextOut_.fileBody_.fileSize_ = st.st_size;

        // close fd after sendfile in writeResponse
        r->contextOut_.fileBody_.filefd_.reset(std::move(fd));

        return PHASE_NEXT;
    }

autoindex:
    if (server.autoIndex_ == 0)
    {
    send404:
        setErrorResponse(r, HTTP_NOT_FOUND);
        return doResponse(r);
    }
    else
    {
        // access a directory but uri doesn't end with '/'
        if (uri.back() != '/')
        {
            setErrorResponse(r, HTTP_MOVED_PERMANENTLY);
            r->contextOut_.headers_.emplace_back("Location", uri + "/");
            return doResponse(r);
        }

        r->contextOut_.resCode_ = HTTP_OK;
        r->contextOut_.statusLine_ = getStatusLineByCode(r->contextOut_.resCode_);
        r->contextOut_.resType_ = ResponseType::AUTO_INDEX;
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

    int val = 1;
    bool isDomain = 0;

    // setup upstream server
    auto &server = serverPtr->servers_[r->c_->serverIdx_];
    int idx = selectServer(r);
    std::string addr = server.to_[idx];

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

    LOG_INFO << "Upstream to " << addr << " -> " << ip << ":" << port << newUri;

    // setup connection
    Connection *upc = serverPtr->pool_.getNewConnection();

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
        serverPtr->pool_.recoverConnection(upc);
        finalizeRequest(r);
        return ERROR;
    }

    if (setsockopt(upc->fd_.getFd(), SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) < 0)
    {
        LOG_WARN << "set keepalived failed";
        serverPtr->pool_.recoverConnection(upc);
        finalizeRequest(r);
        return ERROR;
    }

    upc->addr_.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &upc->addr_.sin_addr);
    upc->addr_.sin_port = htons(port);

    if (connect(upc->fd_.getFd(), (struct sockaddr *)&upc->addr_, sizeof(upc->addr_)) < 0)
    {
        LOG_WARN << "CONNECT ERR, FINALIZE CONNECTION, errro: " << strerror(errno);
        serverPtr->pool_.recoverConnection(upc);
        finalizeRequest(r);
        return ERROR;
    }

    if (setnonblocking(upc->fd_.getFd()) < 0)
    {
        LOG_WARN << "set nonblocking failed";
        serverPtr->pool_.recoverConnection(upc);
        finalizeRequest(r);
        return ERROR;
    }

    // setup upstream
    std::shared_ptr<Upstream> ups(new Upstream());
    r->c_->upstream_ = ups;
    upc->upstream_ = ups;
    ups->client_ = r->c_;
    ups->upstream_ = upc;
    ups->ctx_.upsIdx_ = idx;
    std::shared_ptr<Request> upsr(new Request());
    upsr->c_ = ups->upstream_;
    ups->upstream_->request_ = upsr;

    // setup send content
    auto &wb = upc->writeBuffer_;
    wb.append(r->methodName_.toString() + " " + newUri + " HTTP/1.1\r\n");
    auto &in = r->contextIn_;

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

    // read: empty; write: send2Upstream
    upc->write_.handler_ = send2Upstream;

    // read: clientAliveCheck; write: empty
    r->c_->read_.handler_ = clientAliveCheck;

    if (serverPtr->multiplexer_->addFd(upc->fd_.getFd(), Events(IN | OUT | ET), upc) != 1)
    {
        LOG_CRIT << "epoller addfd failed, error:" << strerror(errno);
        serverPtr->pool_.recoverConnection(upc);
        finalizeRequest(r);
        return ERROR;
    }

    LOG_INFO << "init upstream OK, client: " << r->c_->fd_.getFd() << ", upstream: " << upc->fd_.getFd();

    // send && call send2Upstream at upc's loop
    return OK;
}

int send2Upstream(Event *upcEv)
{
    Connection *upc = upcEv->c_;
    std::shared_ptr<Upstream> ups = upc->upstream_;
    std::shared_ptr<Request> cr = upc->upstream_->client_->request_;
    std::shared_ptr<Request> upsr = ups->upstream_->request_;
    int ret = 0;

    for (; upc->writeBuffer_.allRead() != 1;)
    {
        ret = upc->writeBuffer_.bufferSend(upc->fd_.getFd(), 0);

        if (upc->writeBuffer_.allRead())
        {
            break;
        }

        if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                return AGAIN;
            }
            else
            {
                LOG_WARN << "SEND ERR, FINALIZE CONNECTION";
                finalizeRequest(upsr);
                finalizeRequestNow(cr);
                return ERROR;
            }
        }
        else if (ret == 0)
        {
            LOG_WARN << "Upstream close connection, FINALIZE CONNECTION";
            finalizeRequest(upsr);
            finalizeRequestNow(cr);
            return ERROR;
        }
        else
        {
            continue;
        }
    }

    // remove EPOLLOUT events
    if (serverPtr->multiplexer_->modFd(upc->fd_.getFd(), Events(IN | ET), upc) != 1)
    {
        LOG_CRIT << "epoller modfd failed, error:" << strerror(errno);
        finalizeRequest(upsr);
        finalizeRequestNow(cr);
    }

    // read: recvFromUpstream; write: empty
    upc->write_.handler_ = std::function<int(Event *)>();
    upc->read_.handler_ = recvFromUpstream;

    ups->processHandler_ = processUpsStatusLine;

    LOG_INFO << "Send client data to upstream complete";

    return recvFromUpstream(&upc->read_);
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
        n = upc->readBuffer_.bufferRecv(upc->fd_.getFd(), 0);
        if (n < 0 && errno == EAGAIN)
        {
            LOG_INFO << "recv from upstream EAGAIN";
            return AGAIN;
        }
        else if (n < 0 && errno != EAGAIN)
        {
            LOG_WARN << "Recv error, errno: " << strerror(errno);
            finalizeRequest(upsr);
            finalizeRequestNow(cr);
            return ERROR;
        }
        else if (n == 0)
        {
            LOG_WARN << "Upstream server close connection";
            finalizeRequest(upsr);
            finalizeRequestNow(cr);
            return ERROR;
        }
        else
        {
            // processUpsStatusLine->processHeaders->processBody
            ret = ups->processHandler_(upsr);
            if (ret == AGAIN)
            {
                continue;
            }
            else if (ret == ERROR)
            {
                LOG_WARN << "Process error";
                finalizeRequest(upsr);
                finalizeRequestNow(cr);
                return ERROR;
            }

            // OK
            break;
        }
    }

    LOG_INFO << "Upstream recv done";

    cr->c_->writeBuffer_.append("HTTP/1.1 " + std::string(ups->ctx_.status_.start_, ups->ctx_.status_.end_) + "\r\n");
    for (auto &x : upsr->contextIn_.headers_)
    {
        cr->c_->writeBuffer_.append(x.name_ + ": " + x.value_ + "\r\n");
    }
    cr->c_->writeBuffer_.append("\r\n");
    for (auto &x : upsr->requestBody_.listBody_)
    {
        cr->c_->writeBuffer_.append(x.toString());
    }

    // read: block; write: send2Client
    upc->upstream_->client_->read_.handler_ = blockReading;
    upc->upstream_->client_->write_.handler_ = send2Client;

    // call send2Client at client's loop
    if (serverPtr->multiplexer_->modFd(upc->upstream_->client_->fd_.getFd(), Events(IN | OUT | ET),
                                       upc->upstream_->client_) != 1)
    {
        LOG_CRIT << "epoller modfd failed, error:" << strerror(errno);
        finalizeRequest(upsr);
        finalizeRequestNow(cr);
        return ERROR;
    }

    finalizeRequest(upsr);

    return OK;
}

int staticContentHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Static content handler, FD:" << r->c_->fd_.getFd();
    if (r->contextOut_.resType_ == ResponseType::FILE)
    {
        LOG_INFO << "RES_FILE";
        r->contextOut_.contentLength_ = r->contextOut_.fileBody_.fileSize_;
        std::string etag = getEtag(r->contextOut_.fileBody_.filefd_.getFd());
        if (etag != "")
        {
            r->contextOut_.headers_.emplace_back("Etag", std::move(etag));
        }
    }
    else if (r->contextOut_.resType_ == ResponseType::STRING)
    {
        LOG_INFO << "RES_STR";
        r->contextOut_.contentLength_ = r->contextOut_.strBody_.length();
    }
    else if (r->contextOut_.resType_ == ResponseType::EMPTY)
    {
        LOG_INFO << "RES_EMTPY";
        r->contextOut_.contentLength_ = 0;
    }
    else if (r->contextOut_.resType_ == ResponseType::AUTO_INDEX)
    {
        LOG_INFO << "RES_AUTO_INDEX";
        return PHASE_CONTINUE;
    }

    std::string exten = std::string(r->exten_.data_, r->exten_.data_ + r->exten_.len_);

    r->contextOut_.headers_.emplace_back("Content-Type", getContentType(exten, Charset::UTF_8));
    r->contextOut_.headers_.emplace_back("Content-Length", std::to_string(r->contextOut_.contentLength_));
    r->contextOut_.headers_.emplace_back("Connection", "Keep-Alive");

    return doResponse(r);
}

int autoIndexHandler(std::shared_ptr<Request> r)
{
    LOG_INFO << "Auto index handler, FD:" << r->c_->fd_.getFd();
    if (r->contextOut_.resType_ != ResponseType::AUTO_INDEX)
    {
        LOG_WARN << "Pass auto index handler";
        return PHASE_NEXT;
    }

    // setup
    r->contextOut_.resType_ = ResponseType::STRING;

    extenSave[0] = 'h', extenSave[1] = 't', extenSave[2] = 'm', extenSave[3] = 'l', extenSave[4] = '\0';
    r->exten_.data_ = extenSave;
    r->exten_.len_ = 4;

    // auto index
    auto &out = r->contextOut_;
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
        setErrorResponse(r, HTTP_INTERNAL_SERVER_ERROR);
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
    r->contextOut_.headers_.emplace_back("Content-Type", getContentType("html", Charset::UTF_8));
    r->contextOut_.headers_.emplace_back("Content-Length", std::to_string(out.strBody_.length()));
    r->contextOut_.headers_.emplace_back("Connection", "Keep-Alive");

    return doResponse(r);
}

int appendResponseLine(std::shared_ptr<Request> r)
{
    auto &writebuffer = r->c_->writeBuffer_;
    auto &out = r->contextOut_;

    writebuffer.append(out.statusLine_);

    return OK;
}
int appendResponseHeader(std::shared_ptr<Request> r)
{
    auto &writebuffer = r->c_->writeBuffer_;
    auto &out = r->contextOut_;

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
    auto &out = r->contextOut_;

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

int send2Client(Event *ev)
{
    Connection *c = ev->c_;
    std::shared_ptr<Request> r = c->request_;

    int ret = 0;

    LOG_INFO << "Write to client, FD:" << c->fd_.getFd();

    for (; c->writeBuffer_.allRead() != 1;)
    {
        ret = c->writeBuffer_.bufferSend(c->fd_.getFd(), 0);

        if (c->writeBuffer_.allRead())
        {
            break;
        }

        if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                return AGAIN;
            }
            else
            {
                LOG_WARN << "SEND ERR, FINALIZE CONNECTION";
                finalizeRequest(r);
                return ERROR;
            }
        }
        else if (ret == 0)
        {
            LOG_WARN << "Client close connection, FINALIZE CONNECTION";
            finalizeRequest(r);
            return ERROR;
        }
        else
        {
            continue;
        }
    }

    LOG_INFO << "PROXYPASS RESPONSED";

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
