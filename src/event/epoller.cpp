#include "../headers.h"

#include "../core/core.h"

#include "epoller.h"

extern Cycle *cyclePtr;

std::list<Event *> posted_accept_events;
std::list<Event *> posted_events;

Epoller::Epoller(int max_event)
    : epollfd_(-1), size_(max_event), events_((epoll_event *)malloc(sizeof(epoll_event) * size_))
{
}

Epoller::~Epoller()
{
    if (epollfd_ != -1)
        close(epollfd_);
    free(events_);
}

int Epoller::setEpollFd(int fd)
{
    if (fd < 0)
    {
        return 1;
    }
    epollfd_ = fd;
    return 0;
}

int Epoller::getFd()
{
    return epollfd_;
}

// ctx is Connection*
bool Epoller::addFd(int fd, EVENTS events, void *ctx)
{
    if (fd < 0)
        return 0;
    epoll_event event = {0};
    event.data.ptr = ctx;
    event.events = events2epoll(events);

    return epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event) == 0;
}

// ctx is Connection*
bool Epoller::modFd(int fd, EVENTS events, void *ctx)
{
    if (fd < 0)
        return 0;
    epoll_event event = {0};
    event.data.ptr = ctx;
    event.events = events2epoll(events);
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
    int ret = epoll_wait(epollfd_, events_, size_, timeout_ms);
    if (ret == -1)
    {
        return -1;
    }
    for (int i = 0; i < ret; i++)
    {
        Connection *c = (Connection *)events_[i].data.ptr;

        int revents = events_[i].events;
        if (revents & (EPOLLERR | EPOLLHUP))
        {
            revents |= EPOLLIN | EPOLLOUT;
        }

        if (c->quit == 1)
        {
            goto recover;
        }

        if ((revents & EPOLLIN) && c->read_.handler)
        {
            if (flags & POST_EVENTS)
            {
                if (c->read_.type == ACCEPT)
                {
                    posted_accept_events.push_back(&c->read_);
                }
                else
                {
                    posted_events.push_back(&c->read_);
                }
            }
            else
            {
                c->read_.handler(&c->read_);
            }
        }

        if (c->quit == 1)
        {
            goto recover;
        }

        if ((revents & EPOLLOUT) && c->write_.handler)
        {
            if (flags & POST_EVENTS)
            {
                posted_events.push_back(&c->write_);
            }
            else
            {
                c->write_.handler(&c->write_);
            }
        }

    recover:
        if (c->quit)
        {
            int fd = c->fd_.getFd();
            delFd(fd);
            cyclePtr->pool_->recoverConnection(c);
            LOG_INFO << "Connection recover, FD:" << fd;
        }
    }
    return 0;
}