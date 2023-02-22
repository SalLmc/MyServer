#include "event.h"
#include "../core/core.h"

extern Cycle *cyclePtr;

int setEventTimeout(void *ev)
{
    Event *thisev = (Event *)ev;

    if (thisev->timeout == IGNORE_TIMEOUT)
    {
        LOG_INFO << "Ignore this timeout";
        return 0;
    }

    LOG_INFO << "Timeout triggered";
    thisev->timeout = TIMEOUT;
    thisev->handler(thisev);
    return 1;
}