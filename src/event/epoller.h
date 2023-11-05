#ifndef EPOLLER_H
#define EPOLLER_H

#include "../headers.h"

#include "event.h"

#define POST_EVENTS 1

class Epoller : public Multiplexer
{
  public:
    Epoller(int maxEvent = 1024);
    ~Epoller();

    int setEpollFd(int fd);
    int getFd();
    bool addFd(int fd, EVENTS events, void *ctx);
    bool modFd(int fd, EVENTS events, void *ctx);
    bool delFd(int fd);
    int processEvents(int flags = 0, int timeoutMs = -1);

  private:
    int epollfd_ = -1;
    int size_ = 0;
    epoll_event *events_;
};

#endif