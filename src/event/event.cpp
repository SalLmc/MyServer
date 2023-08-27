#include "../headers.h"

#include "../core/core.h"
#include "epoller.h"
#include "event.h"

extern Cycle *cyclePtr;
extern Epoller epoller;

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
        epoller.delFd(fd);
        cyclePtr->pool_->recoverConnection(thisev->c);
        LOG_INFO << "Connection recover, FD:" << fd;
    }

    return 1;
}