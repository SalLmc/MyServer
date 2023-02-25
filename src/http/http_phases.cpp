#include "http_phases.h"
#include "http.h"
#include <vector>

extern std::unordered_map<std::string, std::string> exten_content_type_map;

std::vector<PhaseHandler> phases{
    {genericPhaseChecker, {passPhaseHandler}}, {genericPhaseChecker, {passPhaseHandler}},
    {genericPhaseChecker, {passPhaseHandler}}, {genericPhaseChecker, {passPhaseHandler}},
    {genericPhaseChecker, {passPhaseHandler}}, {genericPhaseChecker, {passPhaseHandler}},
    {genericPhaseChecker, {passPhaseHandler}}, {genericPhaseChecker, {passPhaseHandler}},
    {genericPhaseChecker, {passPhaseHandler}}, {genericPhaseChecker, {staticContentHandler}},
    {genericPhaseChecker, {passPhaseHandler}}};

PhaseHandler::PhaseHandler(std::function<int(Request *, PhaseHandler *)> checkerr,
                           std::vector<std::function<int(Request *)>> &&handlerss)
    : checker(checkerr), handlers(handlerss)
{
}

static int passPhaseHandler(Request *r)
{
    return NEXT_PHASE;
}

static int staticContentHandler(Request *r)
{
    Connection *c = r->c;
    std::string path = "static" + std::string(r->uri.data, r->uri.data + r->uri.len);
    if (path.back() == '/')
    {
        path += "index.html";
    }
    Fd filefd = open(path.c_str(), O_RDONLY);
    assert(filefd.getFd() >= 0);
    struct stat st;
    fstat(filefd.getFd(), &st);
    std::string exten=std::string(r->exten.data, r->exten.data + r->exten.len);
    if(exten_content_type_map.count(exten))
    {
        r->headers_in
    }
    if (std::string(r->exten.data, r->exten.data + r->exten.len) == "ico")
    {
        char tmp[] = FAVICON_HEADER;
        write(c->fd_.getFd(), tmp, strlen(tmp));
        sprintf(tmp, "%lx\r\n", st.st_size);
        write(c->fd_.getFd(), tmp, strlen(tmp));
    }
    else
    {
        char tmp[] = HTML_HEADER;
        write(c->fd_.getFd(), tmp, strlen(tmp));
        sprintf(tmp, "%lx\r\n", st.st_size);
        write(c->fd_.getFd(), tmp, strlen(tmp));
    }
    sendfile(c->fd_.getFd(), filefd.getFd(), NULL, st.st_size);
    write(c->fd_.getFd(), CRLF, 2);
    write(c->fd_.getFd(), "0\r\n\r\n", 5);
    heap.hDelete(r);
    finalizeConnection(c);
}

int genericPhaseChecker(Request *r, PhaseHandler *ph)
{
    int ret = 0;
    for (int i = 0; i < ph->handlers.size(); i++)
    {
        ret = ph->handlers[i](r);
        if (ret == NEXT_PHASE)
        {
            r->at_phase++;
            return OK;
        }
        if (ret == PHASE_ERR)
        {
            finalizeConnection(r->c);
            return ERROR;
        }
        if (ret == PHASE_CONTINUE)
        {
            continue;
        }
    }
    return AGAIN;
}