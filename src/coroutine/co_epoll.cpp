#include "co_epoll.h"

int co_epoll_wait(int epfd, struct co_epoll_res *events, int maxevents, int timeout)
{
    return epoll_wait(epfd, events->events, maxevents, timeout);
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
    epollfd = epoll_create(co_epoll_t::_EPOLL_SIZE);
    assert(epollfd >= 0);
    timer = new timeout_t(60000);
    list_timeout = (timeout_link_t *)calloc(1, sizeof(timeout_link_t));
    list_active = (timeout_link_t *)calloc(1, sizeof(timeout_link_t));
    result = new co_epoll_res(1024);
}

co_epoll_t::~co_epoll_t()
{
    delete timer;
    free(list_timeout);
    free(list_active);
    delete result;
}