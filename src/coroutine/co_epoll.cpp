#include "co_epoll.h"

int co_epoll_wait(int epfd, co_epoll_res *res, int maxevents, int timeout)
{
    return epoll_wait(epfd, res->events, maxevents, timeout);
}

co_epoll_res::co_epoll_res(int n)
{
    size = n;
    events = (epoll_event *)calloc(1, n * sizeof(epoll_event));
}

co_epoll_res::~co_epoll_res()
{
    if (events != NULL)
    {
        free(events);
    }
}

co_epoll_t::co_epoll_t()
{
    epollfd = epoll_create1(0);
    assert(epollfd >= 0);
    timer = new timeout_t(60000);
    list_timeout = (timeout_link_t *)calloc(1, sizeof(timeout_link_t));
    list_active = (timeout_link_t *)calloc(1, sizeof(timeout_link_t));
    result = new co_epoll_res(_EPOLL_SIZE);
}

co_epoll_t::~co_epoll_t()
{
    delete timer;
    free(list_timeout);
    free(list_active);
    delete result;
}