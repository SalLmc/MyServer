#include "http_phases.h"
#include "../event/epoller.h"
#include "../util/utils_declaration.h"
#include "http.h"
#include <dirent.h>
#include <list>
#include <sys/types.h>
#include <vector>

#include "../memory/memory_manage.hpp"

extern std::unordered_map<std::string, std::string> exten_content_type_map;
extern HeapMemory heap;
extern Epoller epoller;
extern Cycle *cyclePtr;

u_char exten_save[16];

int testPhaseHandler(Request *r)
{
    r->headers_out.status = HTTP_OK;
    r->headers_out.status_line = "HTTP/1.1 200 OK\r\n";
    r->headers_out.restype = RES_STR;
    auto &str = r->headers_out.str_body;
    str.append("<html>\n<head>\n\t<title>404 Not Found</title>\n</head>\n");
    str.append("<body>\n\t<center>\n\t\t<h1>404 Not "
               "Found</h1>\n\t</center>\n\t<hr>\n\t<center>MyServer</center>\n</body>\n</html>");

    r->headers_out.headers.emplace_back("Content-Type", std::string(exten_content_type_map["html"]));
    r->headers_out.headers.emplace_back("Content-Length", std::to_string(str.length()));
    // r->headers_out.headers.emplace_back("Keep-Alive", "timeout=40");
    doResponse(r);

    // LOG_INFO << "PHASE_ERR";
    return PHASE_ERR;
}

std::vector<PhaseHandler> phases{{genericPhaseChecker, {passPhaseHandler}},
                                 {genericPhaseChecker, {passPhaseHandler}},
                                 {genericPhaseChecker, {passPhaseHandler}},
                                 {genericPhaseChecker, {passPhaseHandler}},
                                 {genericPhaseChecker, {passPhaseHandler}},
                                 {genericPhaseChecker, {passPhaseHandler}},
                                 {genericPhaseChecker, {contentAccessHandler}},
                                 {genericPhaseChecker, {passPhaseHandler}},
                                 {genericPhaseChecker, {passPhaseHandler}},
                                 {genericPhaseChecker, {proxyPassHandler, staticContentHandler, autoIndexHandler}},
                                 {genericPhaseChecker, {passPhaseHandler}}};

PhaseHandler::PhaseHandler(std::function<int(Request *, PhaseHandler *)> checkerr,
                           std::vector<std::function<int(Request *)>> &&handlerss)
    : checker(checkerr), handlers(handlerss)
{
}

int genericPhaseChecker(Request *r, PhaseHandler *ph)
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
    }
    // nothing went wrong & no function asks to go to the next phase
    return OK;
}

int passPhaseHandler(Request *r)
{
    return PHASE_NEXT;
}

int contentAccessHandler(Request *r)
{
    auto &server = cyclePtr->servers_[r->c->server_idx_];
    std::string uri = std::string(r->uri.data, r->uri.data + r->uri.len);
    std::string path;
    int fd;

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

        std::string _403_path = cyclePtr->servers_[r->c->server_idx_].root + "/403.html";
        Fd _403fd = open(_403_path.c_str(), O_RDONLY);
        if (_403fd.getFd() < 0)
        {
            r->headers_out.restype = RES_STR;
            auto &str = r->headers_out.str_body;
            str.append("<html>\n<head>\n\t<title>403 Forbidden</title>\n</head>\n");
            str.append("<body>\n\t<center>\n\t\t<h1>404 "
                       "Forbidden</h1>\n\t</center>\n\t<hr>\n\t<center>MyServer</center>\n</body>\n</html>");

            r->headers_out.headers.emplace_back("Content-Type", std::string(exten_content_type_map["html"]));
            r->headers_out.headers.emplace_back("Content-Length", std::to_string(str.length()));
            // r->headers_out.headers.emplace_back("Keep-Alive", "timeout=40");
        }
        else
        {
            r->headers_out.restype = RES_FILE;
            struct stat st;
            fstat(_403fd.getFd(), &st);
            r->headers_out.file_body.filefd = _403fd;
            r->headers_out.file_body.file_size = st.st_size;

            r->headers_out.headers.emplace_back("Content-Type", std::string(exten_content_type_map["html"]));
            r->headers_out.headers.emplace_back("Content-Length", std::to_string(st.st_size));
            // r->headers_out.headers.emplace_back("Keep-Alive", "timeout=40");
        }

        doResponse(r);

        LOG_INFO << "PHASE_ERR";
        return PHASE_ERR;
    }

    if (uri == "/")
    {
        path = server.root + "/" + server.index;
        fd = open(path.c_str(), O_RDONLY);

        if (fd >= 0) // return index if exist
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

        if (fd >= 0)
        {
            struct stat st;
            fstat(fd, &st);
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
                if (fd >= 0)
                {
                    struct stat st;
                    fstat(fd, &st);
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
    if (fd >= 0)
    {
        r->headers_out.status = HTTP_OK;
        r->headers_out.status_line = "HTTP/1.1 200 OK\r\n";
        r->headers_out.restype = RES_FILE;
        // close fd after sendfile in writeResponse
        r->headers_out.file_body.filefd = fd;

        struct stat st;
        fstat(fd, &st);
        r->headers_out.file_body.file_size = st.st_size;

        return PHASE_NEXT;
    }

autoindex:
    if (server.auto_index == 0)
    {
    send404:
        r->headers_out.status = HTTP_NOT_FOUND;
        r->headers_out.status_line = "HTTP/1.1 404 NOT FOUND\r\n";

        std::string _404_path = cyclePtr->servers_[r->c->server_idx_].root + "/404.html";
        Fd _404fd = open(_404_path.c_str(), O_RDONLY);
        if (_404fd.getFd() < 0)
        {
            r->headers_out.restype = RES_STR;
            auto &str = r->headers_out.str_body;
            str.append("<html>\n<head>\n\t<title>404 Not Found</title>\n</head>\n");
            str.append("<body>\n\t<center>\n\t\t<h1>404 Not "
                       "Found</h1>\n\t</center>\n\t<hr>\n\t<center>MyServer</center>\n</body>\n</html>");

            r->headers_out.headers.emplace_back("Content-Type", std::string(exten_content_type_map["html"]));
            r->headers_out.headers.emplace_back("Content-Length", std::to_string(str.length()));
            // r->headers_out.headers.emplace_back("Keep-Alive", "timeout=40");
        }
        else
        {
            r->headers_out.restype = RES_FILE;
            struct stat st;
            fstat(_404fd.getFd(), &st);
            r->headers_out.file_body.filefd = _404fd;
            r->headers_out.file_body.file_size = st.st_size;

            r->headers_out.headers.emplace_back("Content-Type", std::string(exten_content_type_map["html"]));
            r->headers_out.headers.emplace_back("Content-Length", std::to_string(st.st_size));
            // r->headers_out.headers.emplace_back("Keep-Alive", "timeout=40");
        }

        doResponse(r);

        LOG_INFO << "PHASE_ERR";
        return PHASE_ERR;
    }
    else
    {
        r->headers_out.status = HTTP_OK;
        r->headers_out.status_line = "HTTP/1.1 200 OK\r\n";
        r->headers_out.restype = RES_AUTO_INDEX;
        return PHASE_NEXT;
    }
}

int proxyPassHandler(Request *r)
{
    if (r->now_proxy_pass != 1)
    {
        return PHASE_CONTINUE;
    }
    r->now_proxy_pass = 0;

    int ret = readRequestBody(r);
    LOG_INFO << "readRequestBody:" << ret;
    if (ret != OK)
    {
        LOG_INFO << "PHASE ERR";
        return PHASE_ERR;
    }

    auto &server = cyclePtr->servers_[r->c->server_idx_];
    std::string addr = server.proxy_pass.to;
    std::string ip = getIp(addr);
    int port = getPort(addr);
    std::string newUri = getNewUri(addr);

    Connection *upc = initUpstream();

    upc->addr_.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &upc->addr_.sin_addr);
    upc->addr_.sin_port = htons(port);

    if (connect(upc->fd_.getFd(), (struct sockaddr *)&upc->addr_, sizeof(upc->addr_)) < 0)
    {
        LOG_INFO << "PHASE ERR";
        return PHASE_ERR;
    }
    setnonblocking(upc->fd_.getFd());

    // printf("%s\n", std::string(r->request_start, r->request_start + r->request_length).c_str());

    auto &wb = upc->writeBuffer_;
    wb.append("GET " + newUri + " HTTP/1.1\r\n");
    auto &in = r->headers_in;
    for (auto &x : in.headers)
    {
        if (x.name == "Accept-Encoding")
        {
            continue;
        }
        std::string thisHeader = x.name + ": " + x.value + "\r\n";
        wb.append(thisHeader);
    }
    wb.append("\r\n");

    printf("%s\n", wb.allToStr().c_str());

    upc->writeBuffer_.sendFd(upc->fd_.getFd(), &errno, 0);

    int n;
    while (1)
    {
        n = upc->readBuffer_.recvFd(upc->fd_.getFd(), &errno, 0);
        if (n < 0 && errno == EAGAIN)
        {
            continue;
        }
        else if (n < 0)
        {
            printf("%s\n", strerror(errno));
        }
        break;
    }

    for (auto x : upc->readBuffer_.allToStr())
    {
        printf("%c", x);
    }
    Fd fd = open("tmp", O_CREAT | O_RDWR, 0666);
    write(fd.getFd(), upc->readBuffer_.peek(), upc->readBuffer_.readableBytes());
    return testPhaseHandler(r);
}

int staticContentHandler(Request *r)
{
    if (r->headers_out.restype == RES_FILE)
    {
        r->headers_out.content_length = r->headers_out.file_body.file_size;
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
        r->headers_out.headers.emplace_back("Content-Type", std::string(exten_content_type_map[exten]));
    }
    else
    {
        r->headers_out.headers.emplace_back("Content-Type", "application/octet-stream");
    }
    r->headers_out.headers.emplace_back("Content-Length", std::to_string(r->headers_out.content_length));
    // r->headers_out.headers.emplace_back("Keep-Alive", "timeout=40");

    doResponse(r);

    return PHASE_NEXT;
}

int autoIndexHandler(Request *r)
{
    if (r->headers_out.restype != RES_AUTO_INDEX)
    {
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
    static char header[] = "</title><meta charset=\"utf-8\"></head>" CRLF "<body>" CRLF "<h1>Index of ";
    static char tail[] = "</pre>" CRLF "<hr>" CRLF "</body>" CRLF "</html>";

    auto &root = server.root;
    auto subpath = std::string(r->uri.data, r->uri.data + r->uri.len);
    out.str_body.append(title);
    out.str_body.append(header);
    out.str_body.append(subpath);
    out.str_body.append("</h1>" CRLF "<hr>" CRLF);
    out.str_body.append("<pre><a href=\"../\">../</a>" CRLF);

    Dir dir(opendir((root + subpath).c_str()));
    dir.getInfos(root + subpath);
    for (auto &x : dir.infos)
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

    // headers
    std::string exten = std::string(r->exten.data, r->exten.data + r->exten.len);
    if (exten_content_type_map.count(exten))
    {
        r->headers_out.headers.emplace_back("Content-Type", std::string(exten_content_type_map[exten]));
    }
    else
    {
        r->headers_out.headers.emplace_back("Content-Type", "application/octet-stream");
    }
    r->headers_out.headers.emplace_back("Content-Length", std::to_string(out.str_body.length()));
    // r->headers_out.headers.emplace_back("Keep-Alive", "timeout=40");

    doResponse(r);

    return PHASE_NEXT;
}

int appendResponseLine(Request *r)
{
    auto &writebuffer = r->c->writeBuffer_;
    auto &out = r->headers_out;

    writebuffer.append(out.status_line);
    return OK;
}
int appendResponseHeader(Request *r)
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
int appendResponseBody(Request *r)
{
    auto &writebuffer = r->c->writeBuffer_;
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
        writebuffer.append(out.str_body);
    }
    return OK;
}

std::list<std::function<int(Request *)>> responseList{appendResponseLine, appendResponseHeader, appendResponseBody};

int doResponse(Request *r)
{
    for (auto &x : responseList)
    {
        if (x(r) == ERROR)
        {
            return ERROR;
        }
    }

    writeResponse(&r->c->write_);
    return OK;
}
