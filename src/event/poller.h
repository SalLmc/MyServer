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

    bool addFd(int fd, EVENTS events, void *ctx);
    bool modFd(int fd, EVENTS events, void *ctx);
    bool delFd(int fd);
    int processEvents(int flags = 0, int timeoutMs = -1);

  private:
    std::unordered_map<int, PollCtx> fdCtxMap_;
};

#endif