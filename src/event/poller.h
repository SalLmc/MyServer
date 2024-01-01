#ifndef POLLER_H
#define POLLER_H

#include "../headers.h"

#include "event.h"

class Event;

class PollCtx
{
  public:
    PollCtx()
    {
    }
    PollCtx(pollfd pfd, void *ctx) : pfd_(pfd), ctx_(ctx)
    {
    }
    pollfd pfd_;
    void *ctx_;
};

class Poller : public Multiplexer
{
  public:
    Poller();
    ~Poller();

    bool addFd(int fd, Events events, void *ctx);
    bool modFd(int fd, Events events, void *ctx);
    bool delFd(int fd);
    int processEvents(int flags = 0, int timeoutMs = -1);
    void processPostedAcceptEvents();
    void processPostedEvents();

  private:
    std::unordered_map<int, PollCtx> fdCtxMap_;
    std::list<Event *> postedAcceptEvents_;
    std::list<Event *> postedEvents_;
};

#endif