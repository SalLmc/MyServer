#ifndef HTTP_H
#define HTTP

#include "../core/core.h"

class Request;

int initListen(Cycle *cycle, int port);
Connection *addListen(Cycle *cycle, int port);
int finalizeConnection(Connection *c);
int readRequestHeader(Request *r);

// event handler
int newConnection(Event *ev);
int waitRequest(Event *ev);
int processRequestLine(Event *ev);

class Request
{
  public:
    Connection *c;
    // Buffer headerBuffer_;
    int idx_ = -1;
};

class RequestPool
{
  private:
    Request **rPool_;
    uint32_t flags;

  public:
    const static int POOLSIZE = 32;
    RequestPool();
    ~RequestPool();
    Request *getNewRequest();
    void recoverRequest(Request *c);
};

#endif