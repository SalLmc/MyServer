#include "../headers.h"

#include "../core/core.h"

#include "event.h"

extern Server *serverPtr;

int setEventTimeout(void *ev)
{
    Event *thisev = (Event *)ev;

    if (thisev->timeout_ == TimeoutStatus::IGNORE)
    {
        LOG_INFO << "Ignore this timeout";
        return 0;
    }

    // LOG_INFO << "Timeout triggered";
    thisev->timeout_ = TimeoutStatus::TIMEOUT;
    thisev->handler_(thisev);

    if (thisev->c_->quit_)
    {
        int fd = thisev->c_->fd_.getFd();
        serverPtr->multiplexer_->delFd(fd);
        serverPtr->pool_->recoverConnection(thisev->c_);
        LOG_INFO << "Connection recover, FD:" << fd;
    }

    return 1;
}

void processEventsList(std::list<Event *> *events)
{
    while (!events->empty())
    {
        auto &now = events->front();
        if (now->handler_)
        {
            now->handler_(now);
        }
        events->pop_front();
    }
}

uint32_t events2epoll(EVENTS events)
{
    uint32_t e = 0;
    if (events & IN)
        e |= EPOLLIN;
    if (events & OUT)
        e |= EPOLLOUT;
    if (events & HUP)
        e |= EPOLLHUP;
    if (events & ERR)
        e |= EPOLLERR;
    if (events & RDNORM)
        e |= EPOLLRDNORM;
    if (events & WRNORM)
        e |= EPOLLWRNORM;
    if (events & ET)
        e |= EPOLLET;
    return e;
}

short events2poll(EVENTS events)
{
    short e = 0;
    if (events & IN)
        e |= POLLIN;
    if (events & OUT)
        e |= POLLOUT;
    if (events & HUP)
        e |= POLLHUP;
    if (events & ERR)
        e |= POLLERR;
    if (events & RDNORM)
        e |= POLLRDNORM;
    if (events & WRNORM)
        e |= POLLWRNORM;
    return e;
}
