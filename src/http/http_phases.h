#ifndef HTTP_PHASES_H
#define HTTP_PHASES_H

#include "http.h"
#include <functional>

class PhaseHandler
{
  public:
    PhaseHandler() = default;
    PhaseHandler(std::function<int(Request *, PhaseHandler *)> checkerr,
                 std::vector<std::function<int(Request *)>> &&handlerss);
    std::function<int(Request *, PhaseHandler *)> checker;
    std::vector<std::function<int(Request *)>> handlers;
};

#define PHASE_NEXT OK
#define PHASE_ERR ERROR
#define PHASE_CONTINUE AGAIN

#define HTTP_POST_READ_PHASE 0
#define HTTP_SERVER_REWRITE_PHASE 1
#define HTTP_FIND_CONFIG_PHASE 2
#define HTTP_REWRITE_PHASE 3
#define HTTP_POST_REWRITE_PHASE 4
#define HTTP_PREACCESS_PHASE 5
#define HTTP_ACCESS_PHASE 6
#define HTTP_POST_ACCESS_PHASE 7
#define HTTP_PRECONTENT_PHASE 8
#define HTTP_CONTENT_PHASE 9
#define HTTP_LOG_PHASE 10

int genericPhaseChecker(Request *r, PhaseHandler *ph);

int passPhaseHandler(Request *r);
int staticContentHandler(Request *r);
int contentAccessHandler(Request *r);
int autoIndexHandler(Request *r);
int proxyPassHandler(Request *r);

int appendResponseLine(Request *r);
int appendResponseHeader(Request *r);
int appendResponseBody(Request *r);

int doResponse(Request *r);

int initUpstream(Request *r);
int send2upstream(Event *upc_ev);
int upstreamRecv(Event *upc_ev);

#endif