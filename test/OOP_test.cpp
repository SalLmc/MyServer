#include "../src/core/core.h"
#include "../src/http/http.h"
#include "../src/log/logger.h"

Server *serverPtr;
bool enable_logger = 1;
Level logger_level = Level(0);

extern ServerAttribute defaultAttr;

int callback(Event *ev)
{
    std::cout << "new connection\n";
    return 1;
}

int main()
{
    printf("enable_logger:%d\n", enable_logger);
    Server server(new Logger("../log", "test"));
    serverPtr = &server;

    auto attr = defaultAttr;
    attr.port = 8000;

    server.setServers({attr});
    server.initListen(callback);
    server.initEvent(1);
    server.regisListen(Events(IN | ET));

    LOG_INFO << "START";
    while (1)
    {
        server.eventLoop();
    }
}