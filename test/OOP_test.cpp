#include "../src/core/core.h"
#include "../src/log/logger.h"

Server *serverPtr;

int callback(Event *ev)
{
    printf("new connection");
    return 1;
}

int main()
{
    Server server(new Logger("log", "test"));
    serverPtr = &server;

    ServerAttribute attr;
    attr.root_ = "./";
    attr.port_ = 8000;

    server.setServers({attr});
    server.initListen(callback);
    server.initEvent(1);
    server.regisListen();

    LOG_INFO << "START";
    while (1)
    {
        server.eventLoop();
    }
}