#ifndef HTTP_H
#define HTTP

#include "../core/core.h"

Connection *addListen(Cycle *cycle, int port);

// event handler
int newConnection(Event *ev);

#endif