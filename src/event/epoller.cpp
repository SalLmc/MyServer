#include "../event/epoller.h"
#include "../core/core.h"

Epoller::Epoller(int max_event) : epollfd_(epoll_create(512)), events_(max_event)
{
    assert(epollfd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller()
{
    close(epollfd_);
}

int Epoller::getFd()
{
    return epollfd_;
}

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

        if ((events_[i].events & EPOLLIN) && c->idx_!=-1)
        {
            c->read_.handler(&c->read_);
        }
        if ((events_[i].events & EPOLLOUT) && c->idx_!=-1)
        {
            c->write_.handler(&c->write_);
        }
    }
}

int Epoller::getEventFd(size_t i) const
{
    assert(i < events_.size() && i >= 0);
    return events_[i].data.fd;
}

uint32_t Epoller::getEvents(size_t i) const
{
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
}