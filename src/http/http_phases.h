#ifndef HTTP_PHASES_H
#define HTTP_PHASES_H

#include "../headers.h"

class Request;
class Event;

class Phase
{
  public:
    Phase() = default;
    Phase(std::function<int(std::shared_ptr<Request>, Phase *)> phaseHandler,
                 std::vector<std::function<int(std::shared_ptr<Request>)>> &&funcs);
    std::function<int(std::shared_ptr<Request>, Phase *)> phaseHandler;
    std::vector<std::function<int(std::shared_ptr<Request>)>> funcs;
};

#define PHASE_NEXT OK
#define PHASE_ERR ERROR
#define PHASE_CONTINUE AGAIN
#define PHASE_QUIT DONE

int genericPhaseHandler(std::shared_ptr<Request> r, Phase *ph);

int logPhaseFunc(std::shared_ptr<Request> r);
int authAccessFunc(std::shared_ptr<Request> r);
int contentAccessFunc(std::shared_ptr<Request> r);
int proxyPassFunc(std::shared_ptr<Request> r);
int staticContentFunc(std::shared_ptr<Request> r);
int autoIndexFunc(std::shared_ptr<Request> r);

int passPhaseHandler(std::shared_ptr<Request> r);
int endPhaseFunc(std::shared_ptr<Request> r);

int appendResponseLine(std::shared_ptr<Request> r);
int appendResponseHeader(std::shared_ptr<Request> r);
int appendResponseBody(std::shared_ptr<Request> r);

void setErrorResponse(std::shared_ptr<Request> r, ResponseCode code);
int doResponse(std::shared_ptr<Request> r);

int initUpstream(std::shared_ptr<Request> r);
int send2Upstream(Event *upcEv);
int recvFromUpstream(Event *upcEv);
int send2Client(Event *ev);

#endif