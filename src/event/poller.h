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
    PollCtx(pollfd pfd, void *ctx) : pfd(pfd), ctx(ctx)
    {
    }
    pollfd pfd;
    void *ctx;
};

class Poller : public EventProcessor
{
  public:
    Poller();
    ~Poller();

    bool addFd(int fd, EVENTS events, void *ctx);
    bool modFd(int fd, EVENTS events, void *ctx);
    bool delFd(int fd);
    int processEvents(int flags = 0, int timeout_ms = -1);

  private:
    std::unordered_map<int, PollCtx> fdCtxMap_;
};

#endif