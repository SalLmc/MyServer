#include "../src/core/core.h"
#include "../src/event/epoller.h"
#include "../src/http/http.h"
#include "../src/log/logger.h"

Server *serverPtr;
bool enableLogger = 0;
Level loggerLevel = Level(0);

extern ServerAttribute defaultAttr;

int callback(Event *ev)
{
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    Fd fd = accept4(ev->c_->fd_.get(), (sockaddr *)&addr, &len, SOCK_NONBLOCK);

    LinkedBuffer buffer;
    buffer.append("HTTP/1.1 200 OK\r\n");
    buffer.append("Content-Type: text/html; charset=utf-8\r\n");
    buffer.append("Content-Length: 1\r\n");
    buffer.append("Connection: Keep-Alive\r\n\r\n1");

    buffer.bufferSend(fd.get(), 0);
}

int main()
{
    Server server(new Logger("../log", "test"));
    serverPtr = &server;

    auto attr = defaultAttr;
    attr.port = 8000;

    server.setServers({attr});
    server.initListen(callback);
    server.initEvent(1);
    server.regisListen(Events(IN));

    while (1)
    {
        Epoller *ep = (Epoller *)server.multiplexer_;
        int ret = epoll_wait(ep->epollfd_, ep->events_, ep->size_, 1);
        for (int i = 0; i < ret; i++)
        {
            Connection *c = (Connection *)ep->events_[i].data.ptr;
            auto revents = ep->events_[i].events;
            if ((revents & EPOLLIN) && c->read_.handler_)
            {
                c->read_.handler_(&c->read_);
            }
        }
    }
}