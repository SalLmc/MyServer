#include "../src/core/core.h"
#include "../src/log/logger.h"

Server *serverPtr;
bool enable_logger = 1;

int callback(Event *ev)
{
    printf("new connection");
    return 1;
}

int main()
{
    printf("enable_logger:%d\n", enable_logger);
    Server server(new Logger("../log", "test"));
    serverPtr = &server;

    ServerAttribute attr;
    attr.root_ = "./static";
    attr.port_ = 8000;
    attr.index_ = "index.html";

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