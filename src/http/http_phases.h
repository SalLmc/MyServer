#ifndef HTTP_PHASES_H
#define HTTP_PHASES_H

#include "../headers.h"

class Request;
class Event;

class PhaseHandler
{
  public:
    PhaseHandler() = default;
    PhaseHandler(std::function<int(std::shared_ptr<Request>, PhaseHandler *)> checker,
                 std::vector<std::function<int(std::shared_ptr<Request>)>> &&handlers);
    std::function<int(std::shared_ptr<Request>, PhaseHandler *)> checker;
    std::vector<std::function<int(std::shared_ptr<Request>)>> handlers;
};

#define PHASE_NEXT OK
#define PHASE_ERR ERROR
#define PHASE_CONTINUE AGAIN
#define PHASE_QUIT DONE

int genericPhaseChecker(std::shared_ptr<Request> r, PhaseHandler *ph);

int logPhaseHandler(std::shared_ptr<Request> r);
int authAccessHandler(std::shared_ptr<Request> r);
int contentAccessHandler(std::shared_ptr<Request> r);
int proxyPassHandler(std::shared_ptr<Request> r);
int staticContentHandler(std::shared_ptr<Request> r);
int autoIndexHandler(std::shared_ptr<Request> r);

int passPhaseHandler(std::shared_ptr<Request> r);
int endPhaseHandler(std::shared_ptr<Request> r);

int appendResponseLine(std::shared_ptr<Request> r);
int appendResponseHeader(std::shared_ptr<Request> r);
int appendResponseBody(std::shared_ptr<Request> r);

void setErrorResponse(std::shared_ptr<Request> r, int code);
int doResponse(std::shared_ptr<Request> r);

int initUpstream(std::shared_ptr<Request> r);
int send2Upstream(Event *upcEv);
int recvFromUpstream(Event *upcEv);
int send2Client(Event *upcEv);

#endif