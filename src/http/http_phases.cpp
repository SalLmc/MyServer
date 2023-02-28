#include "http_phases.h"
#include "../event/epoller.h"
#include "../util/utils_declaration.h"
#include "http.h"
#include <dirent.h>
#include <list>
#include <sys/types.h>
#include <vector>

#include "../core/memory_manage.hpp"

extern std::unordered_map<std::string, std::string> exten_content_type_map;
extern HeapMemory heap;
extern Epoller epoller;
extern Cycle *cyclePtr;

std::vector<PhaseHandler> phases{
    {genericPhaseChecker, {passPhaseHandler}},     {genericPhaseChecker, {passPhaseHandler}},
    {genericPhaseChecker, {passPhaseHandler}},     {genericPhaseChecker, {passPhaseHandler}},
    {genericPhaseChecker, {passPhaseHandler}},     {genericPhaseChecker, {passPhaseHandler}},
    {genericPhaseChecker, {contentAccessHandler}}, {genericPhaseChecker, {passPhaseHandler}},
    {genericPhaseChecker, {passPhaseHandler}},     {genericPhaseChecker, {staticContentHandler, autoIndexHandler}},
    {genericPhaseChecker, {passPhaseHandler}}};

PhaseHandler::PhaseHandler(std::function<int(Request *, PhaseHandler *)> checkerr,
                           std::vector<std::function<int(Request *)>> &&handlerss)
    : checker(checkerr), handlers(handlerss)
{
}

int genericPhaseChecker(Request *r, PhaseHandler *ph)
{
    int ret = 0;
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
            LOG_INFO << "Phase err";
            finalizeRequest(r);
            return ERROR;
        }
        if (ret == PHASE_CONTINUE)
        {
            continue;
        }
    }
    return AGAIN;
}

int passPhaseHandler(Request *r)
{
    return PHASE_NEXT;
}

int contentAccessHandler(Request *r)
{
    auto &server = cyclePtr->servers_[r->c->server_idx_];
    std::string path;

    if (r->exten.len == 0)
    {
        path = server.root + "/" + server.index;
        u_char *tmp = (u_char *)heap.hMalloc(5);
        tmp[0] = 'h', tmp[1] = 't', tmp[2] = 'm', tmp[3] = 'l', tmp[4] = '\0';
        r->exten.data = tmp;
        r->exten.len = 4;
    }
    else
    {
        path = server.root + std::string(r->uri.data, r->uri.data + r->uri.len);
    }

    int fd = open(path.c_str(), O_RDONLY);

    if (fd >= 0)
    {
        r->headers_out.status = HTTP_OK;
        r->headers_out.status_line = "HTTP/1.1 200 OK\r\n";
        r->headers_out.restype = RES_FILE;
        r->headers_out.file_body.filefd = fd;

        struct stat st;
        fstat(fd, &st);
        r->headers_out.file_body.file_size = st.st_size;

        return PHASE_NEXT;
    }
    else if (server.auto_index == 0)
    {
        r->headers_out.status = HTTP_NOT_FOUND;
        r->headers_out.status_line = "HTTP/1.1 404 NOT FOUND\r\n";

        std::string _404_path = cyclePtr->servers_[r->c->server_idx_].root + "/404.html";
        int _404fd = open(_404_path.c_str(), O_RDONLY);
        if (_404fd < 0)
        {
            r->headers_out.restype = RES_STR;
            auto &str = r->headers_out.str_body;
            str.append("<html>\n<head>\n\t<title>404 Not Found</title>\n</head>\n");
            str.append("<body>\n\t<center>\n\t\t<h1>404 Not "
                       "Found</h1>\n\t</center>\n\t<hr>\n\t<center>MyServer</center>\n</body>\n</html>");

            r->headers_out.headers.emplace_back("Content-Type", std::string(exten_content_type_map["html"]));
            r->headers_out.headers.emplace_back("Content-Length", std::to_string(str.length()));
        }
        else
        {
            r->headers_out.restype = RES_FILE;
            struct stat st;
            fstat(_404fd, &st);
            r->headers_out.file_body.filefd = _404fd;
            r->headers_out.file_body.file_size = st.st_size;

            r->headers_out.headers.emplace_back("Content-Type", std::string(exten_content_type_map["html"]));
            r->headers_out.headers.emplace_back("Content-Length", std::to_string(st.st_size));
        }

        doResponse(r);
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

    u_char *tmp = (u_char *)heap.hMalloc(5);
    tmp[0] = 'h', tmp[1] = 't', tmp[2] = 'm', tmp[3] = 'l', tmp[4] = '\0';
    r->exten.data = tmp;
    r->exten.len = 4;

    // auto index
    auto &out = r->headers_out;
    auto &server = cyclePtr->servers_[r->c->server_idx_];

    static char title[] = "<html>" CRLF "<head><title>Index of ";
    static char header[] = "</title></head>" CRLF "<body>" CRLF "<h1>Index of ";
    static char tail[] = "</pre>" CRLF "<hr>" CRLF "</body>" CRLF "</html>" CRLF;

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
        out.str_body.append(x.name);
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
        writebuffer.append("\r\n");
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

    r->c->write_.handler = writeResponse;
    writeResponse(&r->c->write_);
    epoller.modFd(r->c->fd_.getFd(), EPOLLIN | EPOLLOUT | EPOLLET, r->c);
    return OK;
}
