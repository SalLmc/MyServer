#include "epoller.h"
#include "../core/core.h"
#include "../global.h"

Epoller::Epoller(int max_event) : epollfd_(epoll_create(5)), events_(max_event)
{
    assert(epollfd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller()
{
    if (epollfd_ != -1)
        close(epollfd_);
}

int Epoller::getFd()
{
    return epollfd_;
}

// ctx is Connection*
bool Epoller::addFd(int fd, uint32_t events, void *ctx)
{
    if (fd < 0)
        return 0;
    epoll_event event = {0};
    event.data.ptr = ctx;
    event.events = events;
    return epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event) == 0;
}

bool Epoller::modFd(int fd, uint32_t events, void *ctx)
{
    if (fd < 0)
        return 0;
    epoll_event event = {0};
    event.data.ptr = ctx;
    event.events = events;
    return epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &event) == 0;
}

bool Epoller::delFd(int fd)
{
    if (fd < 0)
        return 0;
    epoll_event event = {0};
    return epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &event) == 0;
}

int Epoller::processEvents(int flags, int timeout_ms)
{
    int ret = epoll_wait(epollfd_, &events_[0], static_cast<int>(events_.size()), timeout_ms);
    for (int i = 0; i < ret; i++)
    {
        Connection *c = (Connection *)events_[i].data.ptr;
        if ((events_[i].events & EPOLLIN) && c != NULL && c->idx_ != -1 && c->read_.handler)
        {
            c->read_.handler(&c->read_);
        }
        if ((events_[i].events & EPOLLOUT) && c != NULL && c->idx_ != -1 && c->write_.handler)
        {
            c->write_.handler(&c->write_);
        }
    }
    return 0;
}