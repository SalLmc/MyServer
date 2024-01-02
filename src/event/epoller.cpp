#include "../headers.h"

#include "../core/core.h"
#include "../log/logger.h"

#include "epoller.h"

extern Server *serverPtr;

Epoller::Epoller(int maxEvent)
    : epollfd_(-1), size_(maxEvent), events_((epoll_event *)malloc(sizeof(epoll_event) * size_))
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
bool Epoller::addFd(int fd, Events events, void *ctx)
{
    if (fd < 0)
        return 0;
    epoll_event event = {0};
    event.data.ptr = ctx;
    event.events = events2epoll(events);

    return epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event) == 0;
}

// ctx is Connection*
bool Epoller::modFd(int fd, Events events, void *ctx)
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

int Epoller::processEvents(int flags, int timeoutMs)
{
    int ret = epoll_wait(epollfd_, events_, size_, timeoutMs);
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

        if (c->quit_ == 1)
        {
            goto recover;
        }

        if ((revents & EPOLLIN) && c->read_.handler_)
        {
            if (flags == POSTED)
            {
                if (c->read_.type_ == EventType::ACCEPT)
                {
                    postedAcceptEvents_.push_back(&c->read_);
                }
                else
                {
                    postedEvents_.push_back(&c->read_);
                }
            }
            else
            {
                c->read_.handler_(&c->read_);
            }
        }

        if (c->quit_ == 1)
        {
            goto recover;
        }

        if ((revents & EPOLLOUT) && c->write_.handler_)
        {
            if (flags == POSTED)
            {
                postedEvents_.push_back(&c->write_);
            }
            else
            {
                c->write_.handler_(&c->write_);
            }
        }

    recover:
        if (c->quit_)
        {
            int fd = c->fd_.getFd();
            delFd(fd);
            serverPtr->pool_.recoverConnection(c);
            LOG_INFO << "Connection recover, FD:" << fd;
        }
    }
    return 0;
}

void Epoller::processPostedAcceptEvents()
{
    while (!postedAcceptEvents_.empty())
    {
        auto &now = postedAcceptEvents_.front();
        if (now->handler_)
        {
            now->handler_(now);
        }
        postedAcceptEvents_.pop_front();
    }
}

void Epoller::processPostedEvents()
{
    while (!postedEvents_.empty())
    {
        auto &now = postedEvents_.front();
        if (now->handler_)
        {
            now->handler_(now);
        }
        postedEvents_.pop_front();
    }
}