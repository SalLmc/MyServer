#ifndef CO_EPOLL_H
#define CO_EPOLL_H

#include "co_timer.h"
#include <memory>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <time.h>

class co_epoll_res
{
  public:
    co_epoll_res() = default;
    co_epoll_res(int n);
    ~co_epoll_res();
    int size;
    epoll_event *events;
};

class co_epoll_t
{
  public:
    co_epoll_t();
    ~co_epoll_t();
    int epollfd;
    static const int _EPOLL_SIZE = 1024;
    timeout_t *timer;
    timeout_link_t *list_timeout;
    timeout_link_t *list_active;
    co_epoll_res *result;
};

int co_epoll_wait(int epfd, co_epoll_res *res, int maxevents, int timeout);

#endif