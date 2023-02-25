#include "http_phases.h"
#include "http.h"
#include <sys/sendfile.h>
#include <vector>

#include "../core/memory_manage.hpp"

extern std::unordered_map<std::string, std::string> exten_content_type_map;
extern HeapMemory heap;

PhaseHandler::PhaseHandler(std::function<int(Request *, PhaseHandler *)> checkerr,
                           std::vector<std::function<int(Request *)>> &&handlerss)
    : checker(checkerr), handlers(handlerss)
{
}

int passPhaseHandler(Request *r)
{
    return NEXT_PHASE;
}

int staticContentHandler(Request *r)
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
    std::string exten = std::string(r->exten.data, r->exten.data + r->exten.len);
    if (exten_content_type_map.count(exten))
    {
        r->headers_out.headers.emplace_back("Content-Type", std::string(exten_content_type_map[exten]));
        r->headers_out.headers.emplace_back("Content-Length", std::to_string(st.st_size));
    }

    write(c->fd_.getFd(), "HTTP/1.1 200 OK\r\n", 17);

    for (auto &x : r->headers_out.headers)
    {
        std::string thisHeader = x.name + ": " + x.value + "\r\n";
        write(c->fd_.getFd(), thisHeader.c_str(), thisHeader.length());
    }

    write(c->fd_.getFd(), "\r\n", 2);
    sendfile(c->fd_.getFd(), filefd.getFd(), NULL, st.st_size);

    heap.hDelete(r);
    finalizeConnection(c);

    return NEXT_PHASE;
}

int genericPhaseChecker(Request *r, PhaseHandler *ph)
{
    int ret = 0;
    for (size_t i = 0; i < ph->handlers.size(); i++)
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

std::vector<PhaseHandler> phases{
    {genericPhaseChecker, {passPhaseHandler}}, {genericPhaseChecker, {passPhaseHandler}},
    {genericPhaseChecker, {passPhaseHandler}}, {genericPhaseChecker, {passPhaseHandler}},
    {genericPhaseChecker, {passPhaseHandler}}, {genericPhaseChecker, {passPhaseHandler}},
    {genericPhaseChecker, {passPhaseHandler}}, {genericPhaseChecker, {passPhaseHandler}},
    {genericPhaseChecker, {passPhaseHandler}}, {genericPhaseChecker, {staticContentHandler}},
    {genericPhaseChecker, {passPhaseHandler}}};