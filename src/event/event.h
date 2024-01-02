#ifndef EVENT_H
#define EVENT_H

#include "../headers.h"

class Server;
class Event;

enum FLAGS
{
    NORMAL,
    POSTED
};

class Multiplexer
{
  public:
    virtual ~Multiplexer(){};
    // ctx is Connection*
    virtual bool addFd(int fd, Events events, void *ctx) = 0;
    // ctx is Connection*
    virtual bool modFd(int fd, Events events, void *ctx) = 0;
    virtual bool delFd(int fd) = 0;
    virtual int processEvents(int flags, int timeoutMs) = 0;
    virtual void processPostedAcceptEvents() = 0;
    virtual void processPostedEvents() = 0;
};

uint32_t events2epoll(Events events);
short events2poll(Events events);

// @param ev Event *
int setEventTimeout(void *ev);

#endif