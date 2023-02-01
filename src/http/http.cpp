#include "http.h"
#include "../event/epoller.h"
#include "../core/core.h"
#include "../global.h"

Connection *addListen(Cycle *cycle, int port)
{
    ConnectionPool *pool = cycle->pool_;
    Connection *listenC = pool->getNewConnection();

    assert(listenC != NULL);

    listenC->addr_.sin_addr.s_addr = INADDR_ANY;
    listenC->addr_.sin_family = AF_INET;
    listenC->addr_.sin_port = htons(port);
    listenC->read_.c = listenC->write_.c = listenC;

    listenC->fd_ = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenC->fd_.getFd() > 0);

    int reuse = 1;
    setsockopt(listenC->fd_.getFd(), SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    setnonblocking(listenC->fd_.getFd());

    assert(bind(listenC->fd_.getFd(), (sockaddr *)&listenC->addr_, sizeof(listenC->addr_)) == 0);

    assert(listen(listenC->fd_.getFd(), 5) == 0);

    return listenC;
}

int newConnection(Event *ev)
{
    Connection *newc = pool.getNewConnection();
    assert(newc != NULL);

    sockaddr_in *addr = &newc->addr_;
    socklen_t len = sizeof(*addr);

    newc->fd_ = accept(ev->c->fd_.getFd(), (sockaddr *)addr, &len);
    assert(newc->fd_.getFd() > 0);

    setnonblocking(newc->fd_.getFd());

    newc->read_.c = newc->write_.c = newc;

    epoller.addFd(newc->fd_.getFd(), EPOLLIN | EPOLLOUT, newc);

    printf("NEW CONNECTION\n");
    return 0;
}