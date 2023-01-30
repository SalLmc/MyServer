#ifndef CO_EPOLL_H
#define CO_EPOLL_H

#include "co_timer.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <time.h>

struct co_epoll_res
{
    int size;
    epoll_event *events;
};

struct co_epoll_t
{
    int epollfd;
    static const int _EPOLL_SIZE = 1024 * 10;
    timeout_t *timer;
    timeout_link_t *list_timeout;
    timeout_link_t *list_active;
    co_epoll_res *result;
};

int co_epoll_wait(int epfd, struct co_epoll_res *events, int maxevents, int timeout);
int co_epoll_ctl(int epfd, int op, int fd, struct epoll_event *ev);
int co_epoll_create(int size);
co_epoll_res *co_epoll_res_alloc(int n);
void co_epoll_res_free(struct co_epoll_res *ptr);

co_epoll_t *alloc_epoll();
void free_epoll( co_epoll_t *ctx );

#endif