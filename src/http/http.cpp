#include "http.h"
#include "../core/memory_manage.hpp"
#include "../event/epoller.h"
#include "../global.h"
#include "../util/utils_declaration.h"

extern ConnectionPool cPool;
extern Epoller epoller;
extern Cycle *cyclePtr;
extern HeapMemory heap;

int initListen(Cycle *cycle, int port)
{
    Connection *listen = addListen(cycle, port);
    cycle->listening_.push_back(listen);

    listen->read_.c = listen;
    listen->read_.handler = newConnection;

    return OK;
}

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

static int recvPrint(Event *ev)
{
    int len = ev->c->readBuffer_.readFd(ev->c->fd_.getFd(), &errno);
    if (len == 0)
    {
        printf("client close connection\n");
        cPool.recoverConnection(ev->c);
        return 1;
    }
    else if (len < 0)
    {
        printf("errno:%d\n", errno);
        cPool.recoverConnection(ev->c);
        return 1;
    }
    printf("%d recv len:%d from client:%s\n", getpid(), len, ev->c->readBuffer_.allToStr().c_str());
    ev->c->writeBuffer_.append(ev->c->readBuffer_.retrieveAllToStr());
    epoller.modFd(ev->c->fd_.getFd(), EPOLLOUT, ev->c);
    return 0;
}

static int echoPrint(Event *ev)
{
    ev->c->writeBuffer_.writeFd(ev->c->fd_.getFd(), &errno);
    ev->c->writeBuffer_.retrieveAll();

    epoller.modFd(ev->c->fd_.getFd(), EPOLLIN, ev->c);
    return 0;
}

int newConnection(Event *ev)
{
    Connection *newc = cPool.getNewConnection();
    assert(newc != NULL);

    sockaddr_in *addr = &newc->addr_;
    socklen_t len = sizeof(*addr);

    newc->fd_ = accept(ev->c->fd_.getFd(), (sockaddr *)addr, &len);

    assert(newc->fd_.getFd() >= 0);

    LOG_INFO << "NEW CONNECTION";

    setnonblocking(newc->fd_.getFd());

    newc->read_.c = newc->write_.c = newc;
    newc->read_.handler = waitRequest;

    epoller.addFd(newc->fd_.getFd(), EPOLLIN, newc);
    return 0;
}

int waitRequest(Event *ev)
{
    Connection *c = ev->c;
    int len = c->readBuffer_.readFd(c->fd_.getFd(), &errno);

    if (len == 0)
    {
        finalizeConnection(c);
        return -1;
    }
    else if (len < 0)
    {
        if (errno != EAGAIN)
        {
            finalizeConnection(c);
            return -1;
        }
        else
        {
            return 0;
        }
    }

    // c->data = rPool.getNewRequest();
    c->data = heap.hNew<Request>();
    ev->handler = processRequestLine;
    processRequestLine(ev);
    return 0;
}

int processRequestLine(Event *ev)
{
    Connection *c = ev->c;
    Request *r = (Request *)c->data;

    int ret = AGAIN;
    for (;;)
    {
        if (ret == AGAIN)
        {
            int rett = readRequestHeader(r);
            if (rett == AGAIN || ret == ERROR)
            {
                break;
            }
        }
    }
}

int readRequestHeader(Request *r)
{
    Connection *c = r->c;
    int n = c->readBuffer_.readFd(c->fd_.getFd(), &errno);
    if (n <= 0)
    {
        finalizeConnection(c);
    }
}

int finalizeConnection(Connection *c)
{
    epoller.delFd(c->fd_.getFd());
    cPool.recoverConnection(c);
    return 0;
}