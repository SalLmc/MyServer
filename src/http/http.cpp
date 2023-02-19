#include "http.h"
#include "../core/memory_manage.hpp"
#include "../event/epoller.h"
#include "../global.h"
#include "../util/utils_declaration.h"
#include "http_parse.h"
#include <string>

extern ConnectionPool cPool;
extern Epoller epoller;
extern Cycle *cyclePtr;
extern HeapMemory heap;

int initListen(Cycle *cycle, int port)
{
    Connection *listen = addListen(cycle, port);
    cycle->listening_.push_back(listen);

    listen->read_.c = listen->write_.c = listen;
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
    else if (len < 0 && errno != EAGAIN)
    {
        printf("errno:%s\n", strerror(errno));
        cPool.recoverConnection(ev->c);
        return 1;
    }
    printf("%d recv len:%d from client:%s\n", getpid(), len, ev->c->readBuffer_.allToStr().c_str());
    ev->c->writeBuffer_.append(ev->c->readBuffer_.retrieveAllToStr());
    epoller.modFd(ev->c->fd_.getFd(), EPOLLOUT | EPOLLET, ev->c);
    return 0;
}

static int echoPrint(Event *ev)
{
    ev->c->writeBuffer_.writeFd(ev->c->fd_.getFd(), &errno);
    ev->c->writeBuffer_.retrieveAll();

    epoller.modFd(ev->c->fd_.getFd(), EPOLLIN | EPOLLET, ev->c);
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

    epoller.addFd(newc->fd_.getFd(), EPOLLIN | EPOLLET, newc);
    return 0;
}

int waitRequest(Event *ev)
{
    LOG_INFO << "wait request";
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

    Request *r = heap.hNew<Request>();
    r->c = c;
    c->data = r;

    ev->handler = processRequestLine;
    processRequestLine(ev);
    return 0;
}

int processRequestLine(Event *ev)
{
    LOG_INFO << "process request line";
    Connection *c = ev->c;
    Request *r = (Request *)c->data;

    int ret = AGAIN;
    for (;;)
    {
        if (ret == AGAIN)
        {
            int rett = readRequestHeader(r);
            LOG_INFO << "readRequestHeader:" << rett;
            if (rett == AGAIN || rett == ERROR)
            {
                break;
            }
        }

        ret = parseRequestLine(r);
        LOG_INFO << "parse request line end, ret:" << ret;
        if (ret == OK)
        {

            /* the request line has been parsed successfully */

            r->request_line.len = r->request_end - r->request_start;
            r->request_line.data = r->request_start;
            r->request_length = r->c->readBuffer_.peek() - (char *)r->request_start;

            r->method_name.len = r->method_end - r->request_start + 1;
            r->method_name.data = r->request_line.data;

            if (r->http_protocol.data)
            {
                r->http_protocol.len = r->request_end - r->http_protocol.data;
            }

            // if (ngx_http_process_request_uri(r) != OK)
            // {
            //     break;
            // }

            if (r->schema_end)
            {
                r->schema.len = r->schema_end - r->schema_start;
                r->schema.data = r->schema_start;
            }

            if (r->host_end)
            {
                r->host.len = r->host_end - r->host_start;
                r->host.data = r->host_start;
            }

            // // HTTP 1.0
            // if (r->http_version < 1000)
            // {
            //     ngx_http_process_request(r);
            //     break;
            // }

            // ev->handler = ngx_http_process_request_headers;
            // ngx_http_process_request_headers(ev);

            break;
        }

        if (ret != AGAIN)
        {
            LOG_CRIT << "parse request line failed";

            finalizeConnection(r->c);
            break;
        }
    }
}

int readRequestHeader(Request *r)
{
    LOG_INFO << "read request header";
    Connection *c = r->c;
    assert(c != NULL);

    int n = c->readBuffer_.beginWrite() - c->readBuffer_.peek();
    if (n > 0)
    {
        return n;
    }

    n = c->readBuffer_.readFd(c->fd_.getFd(), &errno);

    if (n < 0)
    {
        if (errno != EAGAIN)
        {
            finalizeConnection(c);
            return ERROR;
        }
        else
        {
            return AGAIN;
        }
    }
    else if (n == 0)
    {
        LOG_INFO << "client close connection";
        return ERROR;
    }
    return OK;
}

int finalizeConnection(Connection *c)
{
    epoller.delFd(c->fd_.getFd());
    cPool.recoverConnection(c);
    return 0;
}