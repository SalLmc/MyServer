#include "../headers.h"

#include "../core/core.h"
#include "epoller.h"
#include "event.h"

extern Cycle *cyclePtr;

int setEventTimeout(void *ev)
{
    Event *thisev = (Event *)ev;

    if (thisev->timeout == IGNORE_TIMEOUT)
    {
        LOG_INFO << "Ignore this timeout";
        return 0;
    }

    // LOG_INFO << "Timeout triggered";
    thisev->timeout = TIMEOUT;
    thisev->handler(thisev);

    if (thisev->c->quit)
    {
        int fd = thisev->c->fd_.getFd();
        cyclePtr->eventProccessor->delFd(fd);
        cyclePtr->pool_->recoverConnection(thisev->c);
        LOG_INFO << "Connection recover, FD:" << fd;
    }

    return 1;
}

void process_posted_events(std::list<Event *> *events)
{
    while (!events->empty())
    {
        auto &now = events->front();
        if (now->handler)
        {
            now->handler(now);
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
