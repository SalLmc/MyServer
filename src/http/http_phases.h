#ifndef HTTP_PHASES_H
#define HTTP_PHASES_H

#include "../headers.h"

class Request;
class Event;

class PhaseHandler
{
  public:
    PhaseHandler() = default;
    PhaseHandler(std::function<int(std::shared_ptr<Request>, PhaseHandler *)> checkerr,
                 std::vector<std::function<int(std::shared_ptr<Request>)>> &&handlerss);
    std::function<int(std::shared_ptr<Request>, PhaseHandler *)> checker;
    std::vector<std::function<int(std::shared_ptr<Request>)>> handlers;
};

#define PHASE_NEXT OK
#define PHASE_ERR ERROR
#define PHASE_CONTINUE AGAIN
#define PHASE_QUIT DONE

// #define HTTP_POST_READ_PHASE 0
// #define HTTP_SERVER_REWRITE_PHASE 1
// #define HTTP_FIND_CONFIG_PHASE 2
// #define HTTP_REWRITE_PHASE 3
// #define HTTP_POST_REWRITE_PHASE 4
// #define HTTP_PREACCESS_PHASE 5
// #define HTTP_ACCESS_PHASE 6
// #define HTTP_POST_ACCESS_PHASE 7
// #define HTTP_PRECONTENT_PHASE 8
// #define HTTP_CONTENT_PHASE 9
// #define HTTP_LOG_PHASE 10

int genericPhaseChecker(std::shared_ptr<Request> r, PhaseHandler *ph);

int passPhaseHandler(std::shared_ptr<Request> r);
int endPhaseHandler(std::shared_ptr<Request> r);
int logPhaseHandler(std::shared_ptr<Request> r);
int authAccessHandler(std::shared_ptr<Request> r);
int staticContentHandler(std::shared_ptr<Request> r);
int contentAccessHandler(std::shared_ptr<Request> r);
int autoIndexHandler(std::shared_ptr<Request> r);
int proxyPassHandler(std::shared_ptr<Request> r);

int appendResponseLine(std::shared_ptr<Request> r);
int appendResponseHeader(std::shared_ptr<Request> r);
int appendResponseBody(std::shared_ptr<Request> r);

int doResponse(std::shared_ptr<Request> r);

int initUpstream(std::shared_ptr<Request> r);
int send2upstream(Event *upc_ev);
int upstreamRecv(Event *upc_ev);
int upsResponse2Client(Event *upc_ev);

#endif