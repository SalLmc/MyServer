#ifndef EPOLLER_H
#define EPOLLER_H

#include "../headers.h"

#define POST_EVENTS 1

class Event;

void process_posted_events(std::list<Event *> *events);

class Epoller
{
  public:
    Epoller(int max_event = 1024);
    ~Epoller();

    int setEpollFd(int fd);
    int getFd();
    bool addFd(int fd, uint32_t events, void *ctx);
    bool modFd(int fd, uint32_t events, void *ctx);
    bool delFd(int fd);
    int processEvents(int flags = 0, int timeout_ms = -1);

  private:
    int epollfd_ = -1;
    std::vector<epoll_event> events_;
};

#endif